#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

Kiln::Kiln() {}

Cone Kiln::cone() const {
    return cone_;
}

Door Kiln::door() const {
    return door_;
}

int Kiln::stokes() const {
    return stokes_;
}

std::vector<Ware> Kiln::stack() const {
    return stack_;
}

void Kiln::open() {
    // Повторное открытие и открытие на поливном конусе — KilnError.
    if (door_ == Door::Open)
        throw KilnError("is already open");
    if (cone_ == Cone::Glaze)
        throw KilnError("can`t open in glaze");
    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed)
        throw KilnError("is already closed");
    door_ = Door::Closed;
}

void Kiln::fire() {
    // Отказ не тратит такт и не поднимает конус наполовину.
    if (door_ == Door::Open)
        throw KilnError("door must be closed");
    if (cone_ == Cone::Glaze)
        throw KilnError("is alreary at max cone");
    for (const Ware& el: stack_){
        if (el.thickness_mm > kCrackThicknessMm){
            throw KilnError("ware if too thick to fire");
        }
    }
    if (cone_ == Cone::Bisque){
        for (const Ware& el: stack_){
            if (el.program == Program::Bisque){
                throw KilnError("bisque can`t be fired to glaze cone");
            }
        }
    }
    if (cone_ == Cone::Cold)
        cone_ = Cone::Bisque;
    else if (cone_ == Cone::Bisque){
        cone_ = Cone::Glaze;
        for (auto it = seen_glaze_.begin(); it != seen_glaze_.end(); ++it){
            *it = true;
        }
    }
    ++stokes_;
}   

void Kiln::cool() {
    // Ровно один конус вниз. Полива, не видевшая поливного огня, остыть не даёт.
    if (door_ == Door::Open)
        throw KilnError("door must be closed");
    if (cone_ == Cone::Cold)
        throw KilnError("id already cold");
    if (cone_ == Cone::Bisque){
        for (size_t i = 0; i < stack_.size(); ++i){
            if (stack_[i].program == Program::Glaze && !seen_glaze_[i]){
                throw KilnError("must reach glaze cone for start");
            }
        }
    }
    if (cone_ == Cone::Glaze)
        cone_ = Cone::Bisque;
    else if (cone_ == Cone::Bisque)
        cone_ = Cone::Cold;
    ++stokes_;
}

void Kiln::load(const Ware& ware) {
    // Внутри должна оказаться копия. Дверцу load сам не открывает.
    if (door_ == Door::Closed)
        throw KilnError("door must be open");
    if (cone_ == Cone::Glaze)
        throw KilnError("cone must not be glaze");
    if (cone_ == Cone::Bisque){
        if (ware.program != Program::Glaze){
            throw KilnError("ware must be glaze");
        }
    }
    if (validateWare(ware).has_value())
        throw KilnError("not validate");
    for (const Ware& el: stack_){
        if (el.mark == ware.mark) {
            throw KilnError("must not be the same marks");
        }
    }
    if (ware.kind == WareKind::Sagger && !stack_.empty())
        throw KilnError("must be empty");
    for (const Ware& el: stack_){
        if (el.kind == WareKind::Sagger) {
            throw KilnError("only solo sagger");
        }
    }
    if (!fitsInKiln(stack_, ware))
        throw KilnError("change the height");
    stack_.push_back(ware);
    seen_glaze_.push_back(false); 
}   

void Kiln::unload(const std::string& mark) {
    // Снимается только верх стопки. Поливу с утильного конуса не снимают.
    if (door_ == Door::Closed)
        throw KilnError("door must be open");
    if (stack_.empty())
        throw KilnError("kiln is empty");
    if (stack_.back().mark != mark)
        throw KilnError("mark doesn`t match top of stack");
    if (stack_.back().program == Program::Glaze && cone_ != Cone::Cold)
        throw KilnError("glaze only when cold");
    stack_.pop_back();
    seen_glaze_.pop_back();
}

}  // namespace skopin
