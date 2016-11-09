/*
 * Copyright (c) 2016 Advanced Micro Devices, Inc.
 * Copyright (c) 2003-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Steve Reinhardt
 *          Ali Saidi
 *          Brandon Potter
 */

#include "sim/syscall_desc.hh"

#include "base/trace.hh"
#include "config/the_isa.hh"
#include "cpu/base.hh"
#include "cpu/thread_context.hh"
#include "sim/process.hh"
#include "sim/syscall_debug_macros.hh"
#include "sim/syscall_return.hh"

void
SyscallDesc::doSyscall(int callnum, Process *process, ThreadContext *tc)
{
    TheISA::IntReg arg[6] M5_VAR_USED;

    /**
     * Step through the first six parameters for the system call and
     * retrieve their values. Note that index is incremented as a
     * side-effect of the getSyscallArg method which is why the LHS
     * needs the "-1".
     */
    for (int index = 0; index < 6; )
        arg[index - 1] = process->getSyscallArg(tc, index);

    /**
     * Linux supports up to six system call arguments through registers
     * so we want to print all six. Check to the relevant man page to
     * verify how many are actually used by a given system call.
     */
    DPRINTF_SYSCALL(Base, "%s called w/arguments %d, %d, %d, %d, %d, %d\n",
                    _name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);

    /** Invoke the system call */
    SyscallReturn retval = (*executor)(this, callnum, process, tc);

    /**
     * If the system call needs to be restarted, most likely due to
     * blocking behavior, warn that the system call will retry;
     * alternatively, print the return value.
     */
    if (retval.needsRetry())
        DPRINTF_SYSCALL(Base, "%s needs retry\n", _name);
    else
        DPRINTF_SYSCALL(Base, "%s returns %d\n", _name, retval.encodedValue());

    if (!(_flags & SyscallDesc::SuppressReturnValue) && !retval.needsRetry())
        process->setSyscallReturn(tc, retval);
}

bool
SyscallDesc::needWarning()
{
    bool suppress_warning = warnOnce() && _warned;
    _warned = true;
    return !suppress_warning;
}
