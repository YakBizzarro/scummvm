#include <ronin/soundcommon.h>

#define NUM_BUFFERS 4

class OSystem_Dreamcast : public OSystem {

 public:
  // Set colors of the palette
  void set_palette(const byte *colors, uint start, uint num);

  // Set the size of the video bitmap.
  // Typically, 320x200
  void init_size(uint w, uint h);

  // Draw a bitmap to screen.
  // The screen will not be updated to reflect the new bitmap
  void copy_rect(const byte *buf, int pitch, int x, int y, int w, int h);

  // Update the dirty areas of the screen
  void update_screen();

  // Either show or hide the mouse cursor
  bool show_mouse(bool visible);
  
  // Set the position of the mouse cursor
  void set_mouse_pos(int x, int y);
  
  // Set the bitmap that's used when drawing the cursor.
  void set_mouse_cursor(const byte *buf, uint w, uint h, int hotspot_x, int hotspot_y);
  
  // Shaking is used in SCUMM. Set current shake position.
  void set_shake_pos(int shake_pos);

  // Get the number of milliseconds since the program was started.
  uint32 get_msecs();
  
  // Delay for a specified amount of milliseconds
  void delay_msecs(uint msecs);
  
  // Create a thread
  void *create_thread(ThreadProc *proc, void *param);
	
  // Get the next event.
  // Returns true if an event was retrieved.	
  bool poll_event(Event *event);
  
  // Set function that generates samples 
  bool set_sound_proc(void *param, SoundProc *proc, byte sound);
		
  // Poll cdrom status
  // Returns true if cd audio is playing
  bool poll_cdrom();

  // Play cdrom audio track
  void play_cdrom(int track, int num_loops, int start_frame, int end_frame);
  
  // Stop cdrom audio track
  void stop_cdrom();

  // Update cdrom audio status
  void update_cdrom();

  // Quit
  void quit();

  // Set a parameter
  uint32 property(int param, uint32 value);

  static OSystem *create();


 private:

  int _ms_cur_x, _ms_cur_y, _ms_cur_w, _ms_cur_h, _ms_old_x, _ms_old_y;
  int _ms_hotspot_x, _ms_hotspot_y, _ms_visible, _devpoll;
  int _current_shake_pos;
  unsigned char *_ms_buf;
  SoundProc *_sound_proc;
  void *_sound_proc_param;

  unsigned char *screen;
  unsigned short *mouse;
  void *screen_tx[NUM_BUFFERS];
  void *mouse_tx[NUM_BUFFERS];
  int current_buffer;
  unsigned short palette[256];

  short temp_sound_buffer[RING_BUFFER_SAMPLES];

  void checkSound();

  void drawMouse(int xdraw, int ydraw, int w, int h,
		 unsigned char *buf, bool visible);

};

extern int handleInput(struct mapledev *pad,
		       int &mouse_x, int &mouse_y,
		       byte &shiftFlags);
extern void initSound();
extern bool selectGame(GameDetector *d, char *&, char *&, class Icon &);

