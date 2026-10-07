# vroot

**vroot** is a lightweight user-space virtualization system for Linux.

It provides a virtual filesystem and process environment without requiring root privileges. System calls made by a guest process are intercepted and translated so that the process can operate inside a virtual root filesystem.

The project separates virtualization from syscall translation:

* **path** — virtualizes filesystem paths
* **FD** — manages file descriptor virtualization and injection
* **process** — handles process-related virtualization
* **syscall** — intercepts and translates system calls

The goal is to build a small, understandable alternative to tools such as PRoot, while experimenting with how filesystem and process virtualization can be implemented at the system-call level.

> **Status:** Experimental. Not production-ready.
