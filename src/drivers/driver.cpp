#include "driver.h"

Driver::Driver() {}
Driver::~Driver() {}
void Driver::Activate()   {}
void Driver::Deactivate() {}
int  Driver::Reset()      { return 0; }

DriverManager::DriverManager() : count(0) {
    for (int i = 0; i < MAX_DRIVERS; i++)
        drivers[i] = nullptr;
}

void DriverManager::AddDriver(Driver* drv) {
    if (count < MAX_DRIVERS)
        drivers[count++] = drv;
}

void DriverManager::ActivateAll() {
    for (int i = 0; i < count; i++)
        drivers[i]->Activate();
}

void DriverManager::DeactivateAll() {
    for (int i = 0; i < count; i++)
        drivers[i]->Deactivate();
}

Driver* DriverManager::Get(int index) {
    if (index < 0 || index >= count) return nullptr;
    return drivers[index];
}

int DriverManager::Count() const { return count; }
