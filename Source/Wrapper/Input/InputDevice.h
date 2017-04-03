#ifndef INPUT_DEVICE_H
#define INPUT_DEVICE_H

struct InputValues {
   bool up;
   bool down;
   bool left;
   bool right;
   bool a;
   bool b;
   bool start;
   bool select;
};

class InputDevice {
public:
   InputDevice() {}

   virtual ~InputDevice() {}

   virtual InputValues poll() = 0;
};

#endif
