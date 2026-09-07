/*
 * Copyright 2024 SimpleDaemons
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "simple-sftpd/security/password.hpp"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>
#include <cstdlib>
#include <vector>

namespace simple_sftpd {
namespace password {

namespace {

const char kPrefix[] = "$pbkdf2-sha256$";
const char kBase64Alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const std::vector<unsigned char>& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    std::size_t i = 0;
    while (i + 2 < data.size()) {
        const std::uint32_t triple =
            (static_cast<std::uint32_t>(data[i]) << 16) |
            (static_cast<std::uint32_t>(data[i + 1]) << 8) |
            static_cast<std::uint32_t>(data[i + 2]);
        out.push_back(kBase64Alphabet[(triple >> 18) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 12) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 6) & 0x3F]);
        out.push_back(kBase64Alphabet[triple & 0x3F]);
        i += 3;
    }

    const std::size_t remaining = data.size() - i;
    if (remaining == 1) {
        const std::uint32_t triple = static_cast<std::uint32_t>(data[i]) << 16;
        out.push_back(kBase64Alphabet[(triple >> 18) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 12) & 0x3F]);
        out.push_back('=');
        out.push_back('=');
    } else if (remaining == 2) {
        const std::uint32_t triple = (static_cast<std::uint32_t>(data[i]) << 16) |
                                     (static_cast<std::uint32_t>(data[i + 1]) << 8);
        out.push_back(kBase64Alphabet[(triple >> 18) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 12) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 6) & 0x3F]);
        out.push_back('=');
    }

    return out;
}

bool base64Decode(const std::string& text, std::vector<unsigned char>& out) {
    auto value = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };

    if (text.size() % 4 != 0) {
        return false;
    }

    out.clear();
    out.reserve((text.size() / 4) * 3);

    for (std::size_t i = 0; i < text.size(); i += 4) {
        int quad[4];
        std::size_t padding = 0;
        for (std::size_t j = 0; j < 4; ++j) {
            const char c = text[i + j];
            if (c == '=') {
                quad[j] = 0;
                ++padding;
                continue;
            }
            if (padding > 0) {
                return false; // '=' may only appear at the end
            }
            quad[j] = value(c);
            if (quad[j] < 0) {
                return false;
            }
        }

        const std::uint32_t triple = (static_cast<std::uint32_t>(quad[0]) << 18) |
                                     (static_cast<std::uint32_t>(quad[1]) << 12) |
                                     (static_cast<std::uint32_t>(quad[2]) << 6) |
                                     static_cast<std::uint32_t>(quad[3]);
        out.push_back(static_cast<unsigned char>((triple >> 16) & 0xFF));
        if (padding < 2) {
            out.push_back(static_cast<unsigned char>((triple >> 8) & 0xFF));
        }
        if (padding < 1) {
            out.push_back(static_cast<unsigned char>(triple & 0xFF));
        }
    }

    return true;
}

bool derive(const std::string& plaintext,
            const std::vector<unsigned char>& salt,
            std::uint32_t iterations,
            std::vector<unsigned char>& out) {
    out.assign(kDigestBytes, 0);
    return PKCS5_PBKDF2_HMAC(plaintext.data(),
                             static_cast<int>(plaintext.size()),
                             salt.data(),
                             static_cast<int>(salt.size()),
                             static_cast<int>(iterations),
                             EVP_sha256(),
                             static_cast<int>(out.size()),
                             out.data()) == 1;
}

} // namespace

bool constantTimeEquals(const std::string& a, const std::string& b) {
    // CRYPTO_memcmp is only constant time for equal lengths, so fold the length
    // difference in without an early return.
    const std::size_t max_length = std::max(a.size(), b.size());
    unsigned char difference = static_cast<unsigned char>(a.size() ^ b.size());
    for (std::size_t i = 0; i < max_length; ++i) {
        const unsigned char left = i < a.size() ? static_cast<unsigned char>(a[i]) : 0;
        const unsigned char right = i < b.size() ? static_cast<unsigned char>(b[i]) : 0;
        difference |= static_cast<unsigned char>(left ^ right);
    }
    return difference == 0;
}

bool isHashed(const std::string& stored) {
    return stored.compare(0, sizeof(kPrefix) - 1, kPrefix) == 0;
}

std::string hash(const std::string& plaintext, std::uint32_t iterations) {
    if (iterations == 0) {
        iterations = kDefaultIterations;
    }

    std::vector<unsigned char> salt(kSaltBytes);
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
        return {}; // Callers must not fall back to storing plaintext.
    }

    std::vector<unsigned char> digest;
    if (!derive(plaintext, salt, iterations, digest)) {
        return {};
    }

    return std::string(kPrefix) + std::to_string(iterations) + "$" +
           base64Encode(salt) + "$" + base64Encode(digest);
}

bool verify(const std::string& plaintext, const std::string& stored) {
    if (!isHashed(stored)) {
        // Legacy plaintext entry, retained so existing user files keep working
        // until they are migrated on next load.
        return constantTimeEquals(plaintext, stored);
    }

    const std::string body = stored.substr(sizeof(kPrefix) - 1);
    const std::size_t first = body.find('$');
    if (first == std::string::npos) {
        return false;
    }
    const std::size_t second = body.find('$', first + 1);
    if (second == std::string::npos) {
        return false;
    }

    std::uint32_t iterations = 0;
    try {
        const unsigned long parsed = std::stoul(body.substr(0, first));
        if (parsed == 0 || parsed > 100000000UL) {
            return false;
        }
        iterations = static_cast<std::uint32_t>(parsed);
    } catch (const std::exception&) {
        return false;
    }

    std::vector<unsigned char> salt;
    std::vector<unsigned char> expected;
    if (!base64Decode(body.substr(first + 1, second - first - 1), salt) ||
        !base64Decode(body.substr(second + 1), expected)) {
        return false;
    }
    if (salt.empty() || expected.size() != kDigestBytes) {
        return false;
    }

    std::vector<unsigned char> actual;
    if (!derive(plaintext, salt, iterations, actual)) {
        return false;
    }

    return CRYPTO_memcmp(actual.data(), expected.data(), expected.size()) == 0;
}

} // namespace password
} // namespace simple_sftpd
