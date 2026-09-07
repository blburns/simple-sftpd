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

#pragma once

#include <cstdint>
#include <string>

namespace simple_sftpd {

/**
 * Salted password hashing for the local user store.
 *
 * Hashes are stored as a single self-describing field so the iteration count
 * and salt travel with the digest and can be raised later without invalidating
 * existing entries:
 *
 *     $pbkdf2-sha256$<iterations>$<base64 salt>$<base64 digest>
 */
namespace password {

/// Iteration count applied to newly created hashes.
constexpr std::uint32_t kDefaultIterations = 210000;

/// Number of random salt bytes generated per hash.
constexpr std::size_t kSaltBytes = 16;

/// Length of the derived key in bytes.
constexpr std::size_t kDigestBytes = 32;

/// True when `stored` is in the PBKDF2 format above rather than legacy plaintext.
bool isHashed(const std::string& stored);

/**
 * Derives a new salted hash for `plaintext`.
 * Returns an empty string if the platform RNG fails, which callers must treat
 * as an error rather than storing the plaintext.
 */
std::string hash(const std::string& plaintext,
                 std::uint32_t iterations = kDefaultIterations);

/**
 * Verifies `plaintext` against `stored`, which may be either a PBKDF2 hash or
 * a legacy plaintext password. The comparison is constant time in both cases,
 * so a caller cannot distinguish a wrong password from a wrong prefix by timing.
 */
bool verify(const std::string& plaintext, const std::string& stored);

/// Constant-time equality for two byte strings.
bool constantTimeEquals(const std::string& a, const std::string& b);

} // namespace password
} // namespace simple_sftpd
