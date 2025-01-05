# Kernel Oops Report for /dev/faulty

This document details the kernel oops resulting from the command echo “hello_world” > /dev/faulty executed on a running QEMU image.

---

## Command Executed

```bash
echo "hello_world" > /dev/faulty
```

## Kernel Oops Details: 

```
[  151.025850] Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
[  151.026348] Mem abort info:
[  151.026479]   ESR = 0x0000000096000045
[  151.026667]   EC = 0x25: DABT (current EL), IL = 32 bits
[  151.026850]   SET = 0, FnV = 0
[  151.026967]   EA = 0, S1PTW = 0
[  151.027085]   FSC = 0x05: level 1 translation fault
[  151.027275] Data abort info:
[  151.027395]   ISV = 0, ISS = 0x00000045
[  151.027531]   CM = 0, WnR = 1
[  151.027809] user pgtable: 4k pages, 39-bit VAs, pgdp=00000000435ff000
[  151.028050] [0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
[  151.028752] Internal error: Oops: 0000000096000045 [#1] PREEMPT SMP
[  151.029439] Modules linked in: faulty(O) [last unloaded: scull]
[  151.030299] CPU: 1 PID: 449 Comm: sh Tainted: G           O      5.15.166-yocto-standard #1
[  151.030642] Hardware name: linux,dummy-virt (DT)
[  151.031078] pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
[  151.031376] pc : faulty_write+0x18/0x20 [faulty]
[  151.032177] lr : vfs_write+0xf8/0x2a0
[  151.032341] sp : ffffffc0096fbd80
[  151.032460] x29: ffffffc0096fbd80 x28: ffffff8003d33700 x27: 0000000000000000
[  151.032847] x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
[  151.033103] x23: 0000000000000000 x22: ffffffc0096fbdc0 x21: 000000557c7eb9b0
[  151.033665] x20: ffffff800369a700 x19: 0000000000000012 x18: 0000000000000000
[  151.033928] x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
[  151.034183] x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
[  151.034438] x11: 0000000000000000 x10: 0000000000000000 x9 : ffffffc008272558
[  151.034772] x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
[  151.035040] x5 : 0000000000000001 x4 : ffffffc000b97000 x3 : ffffffc0096fbdc0
[  151.035301] x2 : 0000000000000012 x1 : 0000000000000000 x0 : 0000000000000000
[  151.035794] Call trace:
[  151.035946]  faulty_write+0x18/0x20 [faulty]
[  151.036179]  ksys_write+0x74/0x110
[  151.036316]  __arm64_sys_write+0x24/0x30
[  151.036465]  invoke_syscall+0x5c/0x130
[  151.036619]  el0_svc_common.constprop.0+0x4c/0x100
[  151.036806]  do_el0_svc+0x4c/0xc0
[  151.036939]  el0_svc+0x28/0x80
[  151.037068]  el0t_64_sync_handler+0xa4/0x130
[  151.037500]  el0t_64_sync+0x1a0/0x1a4
[  151.037907] Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
[  151.038530] ---[ end trace 703e1ab5086cd2e3 ]---
Segmentation fault

Poky (Yocto Project Reference Distro) 4.0.23 qemuarm64 /dev/ttyAMA0

qemuarm64 login: root
Password: 
Last login: Sun Jan  5 05:47:40 +0000 2025 on /dev/ttyAMA0.
```
