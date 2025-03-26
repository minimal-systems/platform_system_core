// Copyright (C) 2025 Minimal Systems Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "capabilities.h"

#include <sys/capability.h>
#include <sys/prctl.h>
#include <bitset>
#include <map>
#include <memory>
#include <string>
#include "log_new.h"
namespace minimal_systems {
namespace init {

using CapSet = std::bitset<CAP_LAST_CAP + 1>;

static const std::map<std::string, int> cap_map = {
        {"CHOWN", CAP_CHOWN},
        {"DAC_OVERRIDE", CAP_DAC_OVERRIDE},
        {"DAC_READ_SEARCH", CAP_DAC_READ_SEARCH},
        {"FOWNER", CAP_FOWNER},
        {"FSETID", CAP_FSETID},
        {"KILL", CAP_KILL},
        {"SETGID", CAP_SETGID},
        {"SETUID", CAP_SETUID},
        {"SETPCAP", CAP_SETPCAP},
        {"LINUX_IMMUTABLE", CAP_LINUX_IMMUTABLE},
        {"NET_BIND_SERVICE", CAP_NET_BIND_SERVICE},
        {"NET_BROADCAST", CAP_NET_BROADCAST},
        {"NET_ADMIN", CAP_NET_ADMIN},
        {"NET_RAW", CAP_NET_RAW},
        {"IPC_LOCK", CAP_IPC_LOCK},
        {"IPC_OWNER", CAP_IPC_OWNER},
        {"SYS_MODULE", CAP_SYS_MODULE},
        {"SYS_RAWIO", CAP_SYS_RAWIO},
        {"SYS_CHROOT", CAP_SYS_CHROOT},
        {"SYS_PTRACE", CAP_SYS_PTRACE},
        {"SYS_PACCT", CAP_SYS_PACCT},
        {"SYS_ADMIN", CAP_SYS_ADMIN},
        {"SYS_BOOT", CAP_SYS_BOOT},
        {"SYS_NICE", CAP_SYS_NICE},
        {"SYS_RESOURCE", CAP_SYS_RESOURCE},
        {"SYS_TIME", CAP_SYS_TIME},
        {"SYS_TTY_CONFIG", CAP_SYS_TTY_CONFIG},
        {"MKNOD", CAP_MKNOD},
        {"LEASE", CAP_LEASE},
        {"AUDIT_WRITE", CAP_AUDIT_WRITE},
        {"AUDIT_CONTROL", CAP_AUDIT_CONTROL},
        {"SETFCAP", CAP_SETFCAP},
        {"MAC_OVERRIDE", CAP_MAC_OVERRIDE},
        {"MAC_ADMIN", CAP_MAC_ADMIN},
        {"SYSLOG", CAP_SYSLOG},
        {"WAKE_ALARM", CAP_WAKE_ALARM},
        {"BLOCK_SUSPEND", CAP_BLOCK_SUSPEND},
        {"AUDIT_READ", CAP_AUDIT_READ},
        {"PERFMON", CAP_PERFMON},
        {"BPF", CAP_BPF},
        {"CHECKPOINT_RESTORE", CAP_CHECKPOINT_RESTORE},
};

static bool ComputeCapAmbientSupported() {
    return prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_IS_SET, CAP_CHOWN, 0, 0) >= 0;
}

static unsigned int ComputeLastValidCap() {
    unsigned int last_valid_cap = CAP_WAKE_ALARM;
    for (; prctl(PR_CAPBSET_READ, last_valid_cap, 0, 0, 0) >= 0; ++last_valid_cap);
    return last_valid_cap - 1;
}

static bool DropBoundingSet(const CapSet& to_keep) {
    unsigned int last_valid_cap = GetLastValidCap();
    for (size_t cap = 0; cap <= last_valid_cap; ++cap) {
        if (cap < to_keep.size() && to_keep.test(cap)) {
            continue;
        }
        if (cap_drop_bound(static_cast<cap_value_t>(cap)) == -1) {
            LOGE("cap_drop_bound(%zu) failed", cap);
            return false;
        }
    }
    return true;
}

static bool SetProcCaps(const CapSet& to_keep, bool add_setpcap) {
    cap_t caps = cap_init();
    if (!caps) {
        LOGE("cap_init() failed");
        return false;
    }

    cap_clear(caps);
    cap_value_t value[1];
    for (size_t cap = 0; cap < to_keep.size(); ++cap) {
        if (to_keep.test(cap)) {
            value[0] = static_cast<cap_value_t>(cap);
            if (cap_set_flag(caps, CAP_INHERITABLE, 1, value, CAP_SET) != 0 ||
                cap_set_flag(caps, CAP_PERMITTED, 1, value, CAP_SET) != 0) {
                LOGE("cap_set_flag(INHERITABLE|PERMITTED, %zu) failed", cap);
                cap_free(caps);
                return false;
            }
        }
    }

    if (add_setpcap) {
        value[0] = CAP_SETPCAP;
        if (cap_set_flag(caps, CAP_PERMITTED, 1, value, CAP_SET) != 0 ||
            cap_set_flag(caps, CAP_EFFECTIVE, 1, value, CAP_SET) != 0) {
            LOGE("cap_set_flag(PERMITTED|EFFECTIVE, %d) failed", CAP_SETPCAP);
            cap_free(caps);
            return false;
        }
    }

    if (cap_set_proc(caps) != 0) {
        LOGE("cap_set_proc() failed");
        cap_free(caps);
        return false;
    }

    cap_free(caps);
    return true;
}

static bool SetAmbientCaps(const CapSet& to_raise) {
    for (size_t cap = 0; cap < to_raise.size(); ++cap) {
        if (to_raise.test(cap)) {
            if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, cap, 0, 0) != 0) {
                LOGE("prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, %zu) failed", cap);
                return false;
            }
        }
    }
    return true;
}

int LookupCap(const std::string& cap_name) {
    auto it = cap_map.find(cap_name);
    return (it != cap_map.end()) ? it->second : -1;
}

bool CapAmbientSupported() {
    static bool cap_ambient_supported = ComputeCapAmbientSupported();
    return cap_ambient_supported;
}

unsigned int GetLastValidCap() {
    static unsigned int last_valid_cap = ComputeLastValidCap();
    return last_valid_cap;
}

bool SetCapsForExec(const CapSet& to_keep) {
    bool add_setpcap = true;
    if (!SetProcCaps(to_keep, add_setpcap)) {
        LOGE("Failed to apply initial capset");
        return false;
    }

    if (!DropBoundingSet(to_keep)) {
        return false;
    }

    add_setpcap = false;
    if (!SetProcCaps(to_keep, add_setpcap)) {
        LOGE("Failed to apply final capset");
        return false;
    }

    return SetAmbientCaps(to_keep);
}

bool DropInheritableCaps() {
    cap_t caps = cap_get_proc();
    if (!caps) {
        LOGE("cap_get_proc() failed");
        return false;
    }

    if (cap_clear_flag(caps, CAP_INHERITABLE) == -1) {
        LOGE("cap_clear_flag(INHERITABLE) failed");
        cap_free(caps);
        return false;
    }

    if (cap_set_proc(caps) != 0) {
        LOGE("cap_set_proc() failed");
        cap_free(caps);
        return false;
    }

    cap_free(caps);
    return true;
}

}  // namespace init
}  // namespace minimal_systems
