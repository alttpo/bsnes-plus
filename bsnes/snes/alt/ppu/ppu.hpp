class PPU : public Processor, public PPUcounter, public MMIO {
public:
  enum : bool { Threaded = true };
  enum : bool { SupportsLayerEnable = true };
  enum : bool { SupportsFrameSkip = true };
  enum : bool { SupportsVRAMExpansion = false };

  alwaysinline void step(unsigned clocks);
  alwaysinline void synchronize_cpu();

  #include "memory/memory.hpp"
  #include "mmio/mmio.hpp"
  #include "render/render.hpp"
  #include "render/ppux.hpp"

  uint16 *surface;
  uint16 *output;

  uint8 ppu1_version;
  uint8 ppu2_version;

  static void Enter();
  void add_clocks(unsigned clocks);

  uint8 region;
  unsigned line;

  enum { NTSC = 0, PAL = 1 };
  enum { BG1 = 0, BG2 = 1, BG3 = 2, BG4 = 3, OAM = 4, BACK = 5, COL = 5 };
  enum { SC_32x32 = 0, SC_64x32 = 1, SC_32x64 = 2, SC_64x64 = 3 };

  struct {
    bool interlace;
    bool overscan;
  } display;

  struct {
    //$2101
    uint8  oam_basesize;
    uint8  oam_nameselect;
    uint16 oam_tdaddr;

    //$210d-$210e
    uint16 m7_hofs, m7_vofs;

    //$211b-$2120
    uint16 m7a, m7b, m7c, m7d, m7x, m7y;
  } cache;

  alwaysinline bool interlace() const { return display.interlace; }
  alwaysinline bool overscan()  const { return display.overscan;  }
  alwaysinline bool hires()     const { return (regs.pseudo_hires || regs.bg_mode == 5 || regs.bg_mode == 6); }

  uint16 light_table[16][32768];
  uint16 mosaic_table[16][4096];
  void render_line();

  void update_oam_status();
  //required functions
  void scanline();
  void render_scanline();
  void frame();
  void enter();
  void power();
  void reset();

  bool layer_enabled[5][4];
  void layer_enable(unsigned layer, unsigned priority, bool enable);
  unsigned frameskip;
  unsigned framecounter;
  bool frame_skipped() const { return framecounter > 0; }
  unsigned get_frameskip() const { return frameskip; }
  void set_frameskip(unsigned frameskip);

  void serialize(serializer&);
  PPU();
  ~PPU();

  friend class PPUDebugger;
};

#if defined(DEBUGGER)
  #include "debugger/debugger.hpp"
  extern PPUDebugger ppu;
#else
  extern PPU ppu;
#endif
