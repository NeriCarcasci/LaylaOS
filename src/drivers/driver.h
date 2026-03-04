#ifndef __DRIVER_H
#define __DRIVER_H

#include "types.h"

class Driver {
public:
    Driver();
    virtual ~Driver();
    virtual void     Activate();
    virtual void     Deactivate();
    virtual int      Reset();
    virtual uint32_t TypeID() { return 0; }
};

class DriverManager {
public:
    DriverManager();
    void    AddDriver(Driver*);
    void    ActivateAll();
    void    DeactivateAll();
    Driver* Get(int index);
    int     Count() const;
private:
    static const int MAX_DRIVERS = 256;
    Driver* drivers[MAX_DRIVERS];
    int     count;
};

#endif
