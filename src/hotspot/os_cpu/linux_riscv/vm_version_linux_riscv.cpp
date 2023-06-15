/*
 * Copyright (c) 2006, 2023, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2021, Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2023, Rivos Inc. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "precompiled.hpp"
#include "asm/register.hpp"
#include "logging/log.hpp"
#include "riscv_hwprobe.hpp"
#include "runtime/os.hpp"
#include "runtime/os.inline.hpp"
#include "runtime/vm_version.hpp"

#include <asm/hwcap.h>
#include <ctype.h>
#include <sys/auxv.h>

#ifndef HWCAP_ISA_I
#define HWCAP_ISA_I  nth_bit('I' - 'A')
#endif

#ifndef HWCAP_ISA_M
#define HWCAP_ISA_M  nth_bit('M' - 'A')
#endif

#ifndef HWCAP_ISA_A
#define HWCAP_ISA_A  nth_bit('A' - 'A')
#endif

#ifndef HWCAP_ISA_F
#define HWCAP_ISA_F  nth_bit('F' - 'A')
#endif

#ifndef HWCAP_ISA_D
#define HWCAP_ISA_D  nth_bit('D' - 'A')
#endif

#ifndef HWCAP_ISA_C
#define HWCAP_ISA_C  nth_bit('C' - 'A')
#endif

#ifndef HWCAP_ISA_Q
#define HWCAP_ISA_Q  nth_bit('Q' - 'A')
#endif

#ifndef HWCAP_ISA_H
#define HWCAP_ISA_H  nth_bit('H' - 'A')
#endif

#ifndef HWCAP_ISA_V
#define HWCAP_ISA_V  nth_bit('V' - 'A')
#endif

#define read_csr(csr)                                           \
({                                                              \
        register unsigned long __v;                             \
        __asm__ __volatile__ ("csrr %0, %1"                     \
                              : "=r" (__v)                      \
                              : "i" (csr)                       \
                              : "memory");                      \
        __v;                                                    \
})

uint32_t VM_Version::cpu_vector_length() {
  assert(ext_V.enabled(), "should not call this");
  return (uint32_t)read_csr(CSR_VLENB);
}

void VM_Version::setup_cpu_available_features() {

  assert(ext_I.feature_bit() == HWCAP_ISA_I, "Bit for I must follow Linux HWCAP");
  assert(ext_M.feature_bit() == HWCAP_ISA_M, "Bit for M must follow Linux HWCAP");
  assert(ext_A.feature_bit() == HWCAP_ISA_A, "Bit for A must follow Linux HWCAP");
  assert(ext_F.feature_bit() == HWCAP_ISA_F, "Bit for F must follow Linux HWCAP");
  assert(ext_D.feature_bit() == HWCAP_ISA_D, "Bit for D must follow Linux HWCAP");
  assert(ext_C.feature_bit() == HWCAP_ISA_C, "Bit for C must follow Linux HWCAP");
  assert(ext_Q.feature_bit() == HWCAP_ISA_Q, "Bit for Q must follow Linux HWCAP");
  assert(ext_H.feature_bit() == HWCAP_ISA_H, "Bit for H must follow Linux HWCAP");
  assert(ext_V.feature_bit() == HWCAP_ISA_V, "Bit for V must follow Linux HWCAP");

  if (!RiscvHwprobe::probe_features()) {
    os_aux_features();
  }
  char* uarch = os_uarch_additional_features();
  vendor_features();

  char buf[1024] = {};
  if (uarch != nullptr && strcmp(uarch, "") != 0) {
    // Use at max half the buffer.
    snprintf(buf, sizeof(buf)/2, "%s,", uarch);
  }
  os::free((void*) uarch);
  strcat(buf, "rv64");
  int i = 0;
  while (_feature_list[i] != nullptr) {
    if (_feature_list[i]->enabled()) {
      log_debug(os, cpu)("Enabled RV64 feature \"%s\" (%ld)",
             _feature_list[i]->pretty(),
             _feature_list[i]->value());
      // The feature string
      if (_feature_list[i]->feature_string()) {
        const char* tmp = _feature_list[i]->pretty();
        if (strlen(tmp) == 1) {
          strcat(buf, tmp);
        } else {
          // Feature string is expected to be lower case.
          // Turn Zxxx into zxxx
          char prebuf[3] = {};
          assert(strlen(tmp) > 1, "Must be");
          prebuf[0] = '_';
          prebuf[1] = (char)tolower(tmp[0]);
          strcat(buf, prebuf);
          strcat(buf, &tmp[1]);
        }
      }
      // Feature bit
      if (_feature_list[i]->feature_bit() != 0) {
        _features |= _feature_list[i]->feature_bit();
      }
      // Change flag default
      _feature_list[i]->update_flag();
    }
    i++;
  }

  _features_string = os::strdup(buf);
}

void VM_Version::os_aux_features() {
  uint64_t auxv = getauxval(AT_HWCAP);
  int i = 0;
  while (_feature_list[i] != nullptr) {
    if ((_feature_list[i]->feature_bit() & auxv) != 0) {
      _feature_list[i]->enable_feature();
    }
    i++;
  }
}

VM_Version::VM_MODE VM_Version::parse_satp_mode(const char* vm_mode) {
  if (!strcmp(vm_mode, "sv39")) {
    return VM_SV39;
  } else if (!strcmp(vm_mode, "sv48")) {
    return VM_SV48;
  } else if (!strcmp(vm_mode, "sv57")) {
    return VM_SV57;
  } else if (!strcmp(vm_mode, "sv64")) {
    return VM_SV64;
  } else {
    return VM_MBARE;
  }
}

char* VM_Version::os_uarch_additional_features() {
  char* ret = nullptr;
  VM_MODE mode = VM_NOTSET;

  FILE *f = fopen("/proc/cpuinfo", "r");
  if (f == nullptr) {
    return nullptr;
  }

  char buf[512], *p;
  while (fgets(buf, sizeof (buf), f) != nullptr &&
         (mode == VM_NOTSET || ret == nullptr)) {
    if ((p = strchr(buf, ':')) != nullptr) {
      if (mode == VM_NOTSET) {
        if (strncmp(buf, "mmu", sizeof "mmu" - 1) == 0) {
          mode = VM_Version::parse_satp_mode(p);
        }
      }
      if (ret == nullptr) {
        if (strncmp(buf, "uarch", sizeof "uarch" - 1) == 0) {
          ret = os::strdup(p + 2);
          ret[strcspn(p, "\n")] = '\0';
        }
      }
    }
  }
  if (mode == VM_NOTSET) {
    mode = VM_MBARE;
  }
  fclose(f);
  satp_mode.enable_feature(mode);
  return ret;
}

void VM_Version::vendor_features() {
  // JEDEC encoded as ((bank - 1) << 7) | (0x7f & JEDEC)
  static constexpr int RIVOS_MVENDORID = 0x6cf; // JEDEC: 0x4f, Bank: 14

  if (!mvendorid.enabled()) {
    return;
  }
  switch (mvendorid.value()) {
    case RIVOS_MVENDORID:
    rivos_features();
    break;
    default:
    break;
  }
}

void VM_Version::rivos_features() {
  // Enable common features not dependent on marchid/mimpid.
  ext_I.enable_feature();
  ext_M.enable_feature();
  ext_A.enable_feature();
  ext_F.enable_feature();
  ext_D.enable_feature();
  ext_C.enable_feature();
  ext_Q.enable_feature();
  ext_H.enable_feature();
  ext_V.enable_feature();

  ext_Zicbom.enable_feature();
  ext_Zicboz.enable_feature();
  ext_Zicbop.enable_feature();

  ext_Zba.enable_feature();
  ext_Zbb.enable_feature();
  ext_Zbc.enable_feature();
  ext_Zbs.enable_feature();

  ext_Zicsr.enable_feature();
  ext_Zifencei.enable_feature();
  ext_Zic64b.enable_feature();
  ext_Zihintpause.enable_feature();

  unaligned_access.enable_feature(MISALIGNED_FAST);
  satp_mode.enable_feature(VM_SV48);

  // Features dependent on march/mimpid.
  // I.e. march.value() and mimplid.value()
}
