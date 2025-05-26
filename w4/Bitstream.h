#include <vector>
#include <cstring>
#include <stdexcept>
#include <cstdint>
#include <type_traits>

class Bitstream {
public:
    Bitstream() : readPos(0) {}

    template<typename T, typename = std::enable_if_t<std::is_trivially_copyable_v<T>>>
    Bitstream(const T& value) : readPos(0) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        buffer.insert(buffer.end(), bytes, bytes + sizeof(T));
    }

    Bitstream(const void* data, size_t size) : buffer(reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size), readPos(0) {}

    template<typename T>
    void write(const T& value) {
        static_assert(std::is_trivially_copyable<T>::value, "Can only write trivially copyable types");
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        buffer.insert(buffer.end(), bytes, bytes + sizeof(T));
    }

    template<typename T>
    void read(T& out) {
        static_assert(std::is_trivially_copyable<T>::value, "Can only read trivially copyable types");
        if (readPos + sizeof(T) > buffer.size()) {
            throw std::runtime_error("Bitstream: read overflow");
        }
        std::memcpy(&out, buffer.data() + readPos, sizeof(T));
        readPos += sizeof(T);
    }

    template<typename T>
    void write(const T* data, size_t count) {
        static_assert(std::is_trivially_copyable_v<T>, "Can only write trivially copyable types");
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
        buffer.insert(buffer.end(), bytes, bytes + sizeof(T) * count);
    }

    template<typename T>
    void read(T* data, size_t count) {
        static_assert(std::is_trivially_copyable_v<T>, "Can only read trivially copyable types");
        size_t bytesNeeded = sizeof(T) * count;
        if (readPos + bytesNeeded > buffer.size()) {
            throw std::runtime_error("Bitstream: read overflow");
        }
        std::memcpy(data, buffer.data() + readPos, bytesNeeded);
        readPos += bytesNeeded;
    }

    template<typename T>
    void skip() {
        if (readPos + sizeof(T) > buffer.size()) {
            throw std::runtime_error("Bitstream: skip overflow");
        }
        readPos += sizeof(T);
    }

    template<typename T>
    void skip(size_t count) {
        if (readPos + sizeof(T) * count > buffer.size()) {
            throw std::runtime_error("Bitstream: skip overflow");
        }
        readPos += sizeof(T) * count;
    }

    const uint8_t* data() const { return buffer.data(); }
    size_t size() const { return buffer.size(); }

private:
    std::vector<uint8_t> buffer;
    size_t readPos;
};
