#ifndef FONT_H
#define FONT_H

#include <string>
#include <memory>

class Font {
public:
    virtual ~Font() = default;
    virtual int getSize() const = 0;
};
using SharedFont = std::shared_ptr<Font>;

#endif
