//
// Copyright (C) 2019 Assured Information Security, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <hve/arch/intel_x64/vcpu.h>
#include <hve/arch/intel_x64/domain.h>
#include <hve/arch/intel_x64/vmcall/vcpu_op.h>

namespace microv::intel_x64
{

vmcall_vcpu_op_handler::vmcall_vcpu_op_handler(
    gsl::not_null<vcpu *> vcpu
) :
    m_vcpu{vcpu}
{
    using namespace vmcs_n;

    vcpu->add_vmcall_handler({&vmcall_vcpu_op_handler::dispatch, this});
}

void
vmcall_vcpu_op_handler::vcpu_op__create_vcpu(vcpu *vcpu)
{
    try {
        vcpu->init_xstate();
        vcpu->set_rax(bfvmm::vcpu::generate_vcpuid());
        g_vcm->create(vcpu->rax(), get_domain(vcpu->rbx()));
    }
    catchall({
        vcpu->set_rax(INVALID_VCPUID);
    })
}

void
vmcall_vcpu_op_handler::vcpu_op__kill_vcpu(vcpu *vcpu)
{
    try {
        auto child_vcpu = get_vcpu(vcpu->rbx());
        child_vcpu->kill();

        vcpu->set_rax(SUCCESS);
    }
    catchall({
        vcpu->set_rax(FAILURE);
    })
}

void
vmcall_vcpu_op_handler::vcpu_op__destroy_vcpu(vcpu *vcpu)
{
    try {
        g_vcm->destroy(vcpu->rbx(), nullptr);
        vcpu->set_rax(SUCCESS);
    }
    catchall({
        vcpu->set_rax(FAILURE);
    })
}

bool
vmcall_vcpu_op_handler::dispatch(vcpu *vcpu)
{
    if (bfopcode(vcpu->rax()) != __enum_vcpu_op) {
        return false;
    }

    switch (vcpu->rax()) {
        case __enum_vcpu_op__create_vcpu:
            this->vcpu_op__create_vcpu(vcpu);
            return true;

        case __enum_vcpu_op__kill_vcpu:
            this->vcpu_op__kill_vcpu(vcpu);
            return true;

        case __enum_vcpu_op__destroy_vcpu:
            this->vcpu_op__destroy_vcpu(vcpu);
            return true;

        default:
            break;
    };

    throw std::runtime_error("unknown vcpu opcode");
}

}
