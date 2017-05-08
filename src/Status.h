#ifndef STATUS_H
#define STATUS_H

enum Status{
  OFF, //Enable pin is off.
  OFFLINE, //Enable pin is on, but serial is not activated.
  ERROR, //Connection has ben stablished, but GRBL is in an ERROR state
  LOCK, //GRBL is in an ALARM/LOCK state, such as limit hit or not homed.
  HOMING, //GRBL is homing, status petitions are delayed until axes are stopped.
  MOVING, //The motors are moving. This movement cannot be cancelled.
  JOGGING, //GRBL is jogging, The movement can be cancelled.
  IDLE //GRBL is sitting idle, and the position has been reported by the machine.
};

#endif //STATUS_H
