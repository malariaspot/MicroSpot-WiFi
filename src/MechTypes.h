#ifndef TYPES_H
#define TYPES_H

enum Status{
  OFF, //Enable pin is off.
  OFFLINE, //Enable pin is on, but serial is not activated.
  ERROR, //Connection has ben stablished, but GRBL is in an ERROR state
  HOMING, //GRBL is homing, status petitions are delayed until axes are stopped.
  MOVING, //The motors are moving. This movement cannot be cancelled.
  LOCK, //GRBL is in an ALARM/LOCK state, such as limit hit or not homed.
  JOGGING, //GRBL is jogging, The movement can be cancelled.
  IDLE //GRBL is sitting idle, and the position has been reported by the machine.
};

enum MsgType{
  POSITION, //status and position report
  AFFIRMATIVE, //this is an "ok"
  ERRONEOUS, //this is an error report
  ALARM, //ALARM report.
  HANDSHAKE, //Initial handshake, after restart.
  MEMORY, //information fomr EEPROM
  NQMESSAGE, //Non queried message
  STARTUP, // startup routine stored in GRBL
  EMPTYLINE, //GRBL sent an empty line.
  DIRTY // dirty message, unparseable.
};

struct Position{
  String x;
  String y;
};


#endif //TYPES_H
