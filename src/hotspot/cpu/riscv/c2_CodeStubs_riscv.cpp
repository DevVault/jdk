/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2020, 2022, Huawei Technologies Co., Ltd. All rights reserved.
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
#include "opto/c2_CodeStubs.hpp"
#include "opto/c2_MacroAssembler.hpp"
#include "runtime/sharedRuntime.hpp"
#include "runtime/stubRoutines.hpp"

#define __ masm.

int C2SafepointPollStub::max_size() const {
  return 13 * 4;
}

void C2SafepointPollStub::emit(C2_MacroAssembler& masm) {
  assert(SharedRuntime::polling_page_return_handler_blob() != NULL,
         "polling page return stub not created yet");
  address stub = SharedRuntime::polling_page_return_handler_blob()->entry_point();
  RuntimeAddress callback_addr(stub);

  __ bind(entry());
  InternalAddress safepoint_pc(__ pc() - __ offset() + _safepoint_offset);
  __ relocate(safepoint_pc.rspec(), [&] {
    int32_t offset;
    __ la_patchable(t0, safepoint_pc.target(), offset);
    __ addi(t0, t0, offset);
  });
  __ sd(t0, Address(xthread, JavaThread::saved_exception_pc_offset()));
  __ far_jump(callback_addr);
}

int C2EntryBarrierStub::max_size() const {
  // 4 bytes for alignment
  return 8 * 4 + 4;
}

void C2EntryBarrierStub::emit(C2_MacroAssembler& masm) {
  __ bind(entry());

  int32_t offset = 0;
  __ movptr(t0, StubRoutines::riscv::method_entry_barrier(), offset);
  __ jalr(ra, t0, offset);
  __ j(continuation());

  // make guard value 4-byte aligned so that it can be accessed by atomic instructions on RISC-V
  __ align(4);
  __ bind(guard());
  __ relocate(entry_guard_Relocation::spec());
  __ emit_int32(0);  // nmethod guard value
}

#undef __
