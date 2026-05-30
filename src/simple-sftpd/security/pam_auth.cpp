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

#include "simple-sftpd/security/pam_auth.hpp"
#include "simple-sftpd/utils/logger.hpp"

#ifdef HAVE_PAM
#include <security/pam_appl.h>
#include <cstdlib>
#include <string.h>
#endif

namespace simple_sftpd {

#ifdef HAVE_PAM
struct pam_data {
    const char* password;
};

static int pam_conv_func(int num_msg, const struct pam_message** msg,
                         struct pam_response** resp, void* appdata_ptr) {
    struct pam_data* data = static_cast<struct pam_data*>(appdata_ptr);
    *resp = static_cast<struct pam_response*>(calloc(num_msg, sizeof(struct pam_response)));

    for (int i = 0; i < num_msg; ++i) {
        switch (msg[i]->msg_style) {
            case PAM_PROMPT_ECHO_OFF:
            case PAM_PROMPT_ECHO_ON:
                (*resp)[i].resp = strdup(data->password);
                (*resp)[i].resp_retcode = PAM_SUCCESS;
                break;
            default:
                free((*resp)[i].resp);
                (*resp)[i].resp = nullptr;
                break;
        }
    }

    return PAM_SUCCESS;
}

static struct pam_conv conv = {
    pam_conv_func,
    nullptr
};
#endif

PAMAuth::PAMAuth(std::shared_ptr<Logger> logger)
    : logger_(logger), pam_available_(false) {
#ifdef HAVE_PAM
    pam_available_ = true;
    logger_->info("PAM authentication available");
#else
    logger_->warn("PAM not available (libpam development headers not found at build time)");
#endif
}

PAMAuth::~PAMAuth() = default;

bool PAMAuth::authenticate(const std::string& username, const std::string& password) {
#ifdef HAVE_PAM
    if (!pam_available_) {
        return false;
    }

    struct pam_data data;
    data.password = password.c_str();
    conv.appdata_ptr = &data;

    pam_handle_t* handle = nullptr;
    int ret = pam_start("simple-sftpd", username.c_str(), &conv, &handle);
    if (ret != PAM_SUCCESS) {
        logger_->error("PAM start failed: " + std::string(pam_strerror(handle, ret)));
        return false;
    }

    ret = pam_authenticate(handle, 0);
    if (ret != PAM_SUCCESS) {
        logger_->warn("PAM authentication failed for user: " + username);
        pam_end(handle, ret);
        return false;
    }

    ret = pam_acct_mgmt(handle, 0);
    if (ret != PAM_SUCCESS) {
        logger_->warn("PAM account management failed for user: " + username);
        pam_end(handle, ret);
        return false;
    }

    logger_->info("PAM authentication successful for user: " + username);
    pam_end(handle, PAM_SUCCESS);
    return true;
#else
    (void)username;
    (void)password;
    return false;
#endif
}

} // namespace simple_sftpd
