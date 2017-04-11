#ifndef STATUS_H
#define STATUS_H

enum Status{
  OFF, //Enable pin is off.
  OFFLINE, //Enable pin is on, but serial is not activated.
  ERROR, //Connection has ben stablished, but GRBL is in an ERROR state
  LOCK, //GRBL is in an ALARM/LOCK state, such as limit hit or not homed.
  MOVING, //The motors are expected to be moving.
  OUTDATED, //The motors may be stopped, but the position reported may not be correct.
  IDLE //GRBL is sitting idle, and the position has been reported by the machine.
};

#endif //STATUS_H