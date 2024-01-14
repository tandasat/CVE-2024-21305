# CVE-2024-21305

This repo contains the report and PoC of [CVE-2024-21305](https://msrc.microsoft.com/update-guide/vulnerability/CVE-2024-21305), the non-secure Hypervisor-Protected Code Integrity (HVCI) configuration vulnerability. This vulnerability allowed arbitrary kernel-mode code execution, effectively bypassing HVCI, within the root partition. For the root cause, read the [blog post](https://tandasat.github.io/blog/2024/01/15/CVE-2024-21305.html) coauthored with Andrea Allievi ([@aall86](https://twitter.com/aall86)), a Windows Core OS engineer who analyzed and fixed the issue.

The [report](./Report/README.md) in this repo is what I sent to MSRC, which contains the PoC and an initial analysis of the issue.

[![Demo](https://img.youtube.com/vi/JrGI_4HgY4c/0.jpg)](https://www.youtube.com/watch?v=JrGI_4HgY4c)


## Timeline

- July 2, Satoshi consulted Andrea for the validity of the bug.
- July 16, Satoshi sent an initial report to Andrea.
- July 20, Satoshi submitted a formal report to MSRC.
- Aug 31, Satoshi agreed with the disclosure day to be January 9th, 2024.
- Oct 17, MSRC notified Satoshi that the report was in the scope of bug bounty and eligible for 1000 USD.
- January 9, 2024, MSFT disclosed and released the fix for the issue.

Thanks MSRC for transparent communication and the engineering team, specifically Andrea, for fixing this issue.
