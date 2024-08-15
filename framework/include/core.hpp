
#ifndef CORE_HPP
#define CORE_HPP

#include <memory> // std::enable_shared_from_this

namespace vks {
    
    template <typename T>
    struct ManagedObject : public std::enable_shared_from_this<T> {
        virtual ~ManagedObject() = default;
    };
    
}

#endif // CORE_HPP
