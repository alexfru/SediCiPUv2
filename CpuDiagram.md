# SediCiPU 2: Logical implementation

The logical implementation can be approximately depicted something like this:

                                  +--------------------+
                                  |                    |
                       +-------+  |     +----------+   |   +-------------+    misc
                       |2-bit  |--+     |Compressor|   +-->|  16KB ROM   |--> ctrl
                       |counter|     +->|to 11 bits|------>|(12->32 bits)|    signals
                       +-------+     |  +----------+       +-------------+
                                     |
                       +-------+     |
            IRQ ins -->|IRQ    |<----|----------------------------------+
                       |   ctrl|     |                                  |
                       +-------+     |     +--------+                   |
                                     +---->| Cond   |<------------------|---+
                                     |     |   eval |---+               |   |
                                     |     +--------+   |               v   |
                                     |                  |    +---+  +-----+ |
                       +-------------|------------------|--->|Mux|->|Flags|-+
                       |             |                  | +->|0-1|  | Reg | |
                       |  +-------+  |  +------------+  | |  +---+  +-----+ |
                       |  | Instr |  |  |Imm  decoder|<-+ |                 |
                       +->|  Reg  |--+->|  Mux 0-7   |-+  +--------+ +------+
                       |  +-------+     +------------+ |           | |      |
                      D|                               |  +---+    | v      |
        +--------+    a|    +---------------+---+      +->|Mux|  +-----+    |
        | Memory |    t|    |               |Mux|--+----->|0-1|->| ALU |    |
        |(16-bit |    a|    +---+          O|0-7|  |      +---+  |     |    |
    P+->|wide, in|<--->+--->|Mux|I   Reg   u+---+  |             |     |--+ |
    h|  |dividual|    B^    |0-7|n  File   t+---+  |             |     |  | |
    y|  |byte  ac|    u|    +---+          s|Mux|  |             |     |  | |
    s|  |cessble)|    s|    |         PC    |0-7|--|-+        +->|     |  | |
     |  +--------+     |    +---------------+---+  | |        |  +-----+  | |
    A|                 |              |            | |        |           | |
    d|                 |  +------+    |            | |        +--------+  | |
    d|                 +--|Buffer|<---|------------+ |  +---+    +---+ |  | |
    r|                 |  +------+    |              +->|Mux|--->| & |-+  | |
    e|                 |              |              +->|0-1| +->|   |    | |
    s|  +---------+    |  +------+    |              |  +---+ |  +---+    | |
    s+--| Mem Sel |<-->+--|Buffer|<---|--------------|--------|---------+-|-+
        |   Regs  |    |  +------+    |              |        |  +---+  | |
        |   ______|    |              |              |        |  |"F"|<-+ |
    L+->|  |Mux0-7|<---|--------------|----------+   |        +--|   |<-  |
    o|  +---------+    |  +------+    |          |   |           +---+    |
    g|A                +--|Buffer|<---|----------+---|--------------------+
    i|d                   +------+    |          |   |                    |
    c|d                +---+          |  +---+   |   |           +-----+  |
    a|r                |Mux|<---------+  |Mux|<--+   |           |Delay|  |
    l+-----------------|0-1|<------------|0-1|<------+-----------| Reg |<-+
                       +---+             +---+                   +-----+

N.B. Control signals not shown.  
N.B. PC is a counter, not a flip-flop.