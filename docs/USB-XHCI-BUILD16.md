# xHCI Ring Transport

Bounded 64-entry command/event TRB rings, No-op command submission, cycle transitions, FIFO completion pointer validation, and event dequeue accounting. The kernel exposes a software-only ring self-test; no controller is activated.

124,000 command submissions/completions, full-ring backpressure, wrap and cycle changes, invalid completion pointers and error completion (UBSan).

`xhci-ring-test` runs the software ring test; it does not detect or drive a USB device. Controller transport is not connected to the PCI hardware backend, and no new Live disk writes or DMA activation are permitted.

Reference: [xHCI specification, Intel](https://www.intel.com/content/www/us/en/products/docs/io/universal-serial-bus/extensible-host-controler-interface-usb-xhci.html).
