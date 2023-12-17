- [Summary](#summary)
- [Platform and requirements](#platform-and-requirements)
- [Description](#description)
  - [Background and the issue](#background-and-the-issue)
  - [Analysis of the cause](#analysis-of-the-cause)
- [PoC](#poc)
  - [Demo configuration](#demo-configuration)
  - [Steps to repro](#steps-to-repro)
  - [Expected result](#expected-result)
  - [Actual result](#actual-result)
- [References](#references)


# Summary

There can be writable and executable guest physical memory regions under HVCI, and kernel-mode code in the root partition could write and execute arbitrary code in kernel-mode.


# Platform and requirements

- Tested on Windows build 10.0.22621.1928
- Intel hardware models with VT-d enabled
  - The exact conditions to see the issue is unclear. See the [Analysis of the cause](#analysis-of-the-cause) section.
- Ability to perform arbitrary kernel-memory read and write for exploitation in the root partition.


# Description

## Background and the issue

When the HVCI is enabled, the root partition is unable to modify executable memory even with the kernel-mode privileges. This is implemented by enforcing W^X memory permission using SLAT. However, under some configurations, memory permissions are set as W+X for some GPAs, making it possible to write and execute arbitrary code in kernel-mode.

[xps15_dump_ept.txt](./xps15_dump_ept.txt) is dump of SLAT (EPT) configuration taken through a hypervisor debugging session using the hvext[1] debugger extension. See the following GPA ranges:

```
GPA                            PA           Flags
...
  0x6ce51000 -   0x6ce71000 -> Identity     -PVU-6XWR   (1)
...
  0x6cf05000 -   0x6cf85000 -> Identity     -PVU-6XWR   (1)
...
  0x7b000000 -   0x7f800000 -> Identity     ---U-0XWR   (2)
```

1. 0x6ce51000 - 0x6ce70fff and 0x6cf05000 - 0x6cf84fff: marked as `U-6XWR`, indicating user- and kernel-mode executable (`U`, `X`), writable (`W`), readable (`R`), write-back (`6`) memory.
2. 0x7b000000 - 0x7f7fffff: marked as `U-0XWR`, indicating the same as above except being uncacheable (`0`).


## Analysis of the cause

Those regions correspond to the reserved memory region indicated by the DMAR ACPI table[4]. Memory type difference (`6` vs `0`) above appears to be caused by the memory map UEFI reports. The below is an excerpt of output of the `memmap` UEFI shell command taken on the same system.
```
Type       Start            End              # Pages          Attributes
Reserved   000000006B80D000-000000006CF84FFF 0000000000001778 000000000000000F  (1')
...
Reserved   0000000078E00000-000000007F7FFFFF 0000000000006A00 0000000000000000  (2')
```
Notice that (1') has Attributes 0xF, which means it can be any memory type, while (2') has 0x0, which seems to mean "do not access". The complete output is uploaded as [xps15_memmap.txt](./xps15_memmap.txt).

Also a few observations:
- The issue persists regardless of if kernel or hypervisor debugging is enabled.
- The uncacheable W+X region appears to be non-exploitable. Accessing this region immediately caused reset.
- The reserved memory regions are not always marked as W+X.
  - On some models, they are marked as `--0-WR`, only readable, writable memory. 3 out of 7 Intel devices I tested had the W+X ranges. 2 out of those 3 had W+X write-back memory.
  - This is probably related to how UEFI reports those regions through the memory map.


# PoC

The demo below remaps LA of `beep!BeepOpen` onto one of the W+X GPA, write shell-code there, and execute it by calling `Beep` Win32 API.

- A recording of reproduction is on [Youtube](https://youtu.be/JrGI_4HgY4c)
- Source code of PoC is in the [wx](./wx/) directory

The exploit assumes that it knows GPA `0x6ce51000` is W+X and hard-codes this address, and thus, not functional on any other system than the setup described below. This could be improved by parsing the DMAR ACPI table.


## Demo configuration

- Windows build 10.0.22621.1992
- Dell XPS 15 7590 (i9-9980HK)
- HVCI is enabled
- No kernel or hypervisor debugging is enabled
- Test-signing is enabled and secure boot is disabled
    - This is because using my own driver instead of a vulnerable driver is much clearer to demonstrate the issue.
- Livekd[2] is installed


## Steps to repro

1. Check the linear address of `beep!BeepOpen` and its PTE using `livekd`
   1. Start command prompt with the administrator privilege
   2. Run those commands:
      ```
      >livekd
      ...

      0: kd> !pte beep!BeepOpen
                                                VA fffff80050fd1510
      PXE at FFFFFBFDFEFF7F80    PPE at FFFFFBFDFEFF0008    PDE at FFFFFBFDFE001438    PTE at FFFFFBFC00287E88
      contains 000000012870B063  contains 000000005BD3B063  contains 0A00000872DFF863  contains 09000007E9369021
      pfn 12870b    ---DA--KWEV  pfn 5bd3b     ---DA--KWEV  pfn 872dff    ---DA--KWEV  pfn 7e9369    ----A--KREV
      ```
   3. Take note of the linear addresses, ie:
      - `fffff80050fd1510` for `beep!BeepOpen`
      - `FFFFFBFC00287E88` for its PTE
2. Update and compile the PoC file. This step can be done on other machine
   1. Open the `wx.sln` on Visual Studio 2022
   2. Update those lines with what we noted at step 1.3
      ```cpp
      auto addrOfBeepOpen = (void*)0xfffff80050fd1510;
      auto pte = (Pte*)0xFFFFFBFC00287E88;
      ```
   3. Build the solution for x64 / Debug
   4. If needed, copy over the compiled `wx.sys` and `beep.exe` into the target system
3. Run the PoC
   1. Optionally, start DbgView[3] with the administrator privileges and enable "Capture Kernel"
   2. Run the following command on the command prompt with the administrators privileges
      ```
      > sc create wx type= kernel binPath= <full_path_to_wx.sys>
      [SC] CreateService SUCCESS
      ```
   3. Load the `wx.sys` and attempt to write executable code
      ```
      > sc start wx
      [SC] StartService FAILED 995:

      The I/O operation has been aborted because of either a thread exit or an application request
      ```
      The error message is expected. If DbgView is running, it should show messages like this:
      ```
      Remapped LA FFFFF80050FD1510 to shell code at GPA 6ce51
      Run the beep.exe to execute shell code
      DriverEntry failed 0xc0000120 for driver \REGISTRY\MACHINE\SYSTEM\ControlSet001\Services\wx
      ```
   4. Run `beep.exe` and attempt to execute generated code


## Expected result

Bug check occurs at step 3.3, pointing `wx.sys` as an offender. The root partition is unable to write code.


## Actual result

Bug check occurs at step 3.4, pointing `beep.sys` as an offender. The crash dump shows all general purpose registers are set to 4141414141414141, indicating the root partition wrote and executed code in kernel-mode.

```
...
CONTEXT:  ffff810a2b96e910 -- (.cxr 0xffff810a2b96e910)
rax=4141414141414141 rbx=4141414141414141 rcx=4141414141414141
rdx=4141414141414141 rsi=4141414141414141 rdi=4141414141414141
rip=fffff80050fd1544 rsp=ffff810a2b96f338 rbp=4141414141414141
 r8=4141414141414141  r9=4141414141414141 r10=4141414141414141
r11=4141414141414141 r12=4141414141414141 r13=4141414141414141
r14=4141414141414141 r15=4141414141414141
iopl=0         nv up ei pl zr na po nc
cs=0010  ss=0018  ds=002b  es=002b  fs=0053  gs=002b             efl=00050246
Beep!BeepOpen+0x34:
Page 6ce51 not present in the dump file. Type ".hh dbgerr004" for details
Page 6ce51 not present in the dump file. Type ".hh dbgerr004" for details
fffff800`50fd1544 ??              ???
...
```


# References

1. https://github.com/tandasat/hvext
2. https://learn.microsoft.com/en-us/sysinternals/downloads/livekd
3. https://learn.microsoft.com/en-us/sysinternals/downloads/debugview
4. https://www.intel.com/content/www/us/en/content-details/774206/intel-virtualization-technology-for-directed-i-o-architecture-specification.html
