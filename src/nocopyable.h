#pragma once

namespace tnet {

class nocopyable {
protected:
    nocopyable() {}
    ~nocopyable() {}
private:
    nocopyable(const nocopyable&);
    nocopyable& operator=(const nocopyable&);
};

}
