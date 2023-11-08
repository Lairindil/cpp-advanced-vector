#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <algorithm>

#include <stdexcept>


template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }
    
    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory(RawMemory&& other) noexcept
        : buffer_(nullptr)
        , capacity_(0) {
            Swap(other);
    }
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
     //   assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    
    Vector() = default;
    explicit Vector(size_t size);
    Vector(const Vector& other);
    Vector(Vector&& other) noexcept;
    
    ~Vector();
    
    using iterator = T*;
        using const_iterator = const T*;
        
        iterator begin() noexcept;
        iterator end() noexcept;
        const_iterator begin() const noexcept;
        const_iterator end() const noexcept;
        const_iterator cbegin() const noexcept;
        const_iterator cend() const noexcept;

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.Size() > data_.Capacity()) {
                /* Применить copy-and-swap */
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            } else {
                /* Скопировать элементы из rhs, создав при необходимости новые
                   или удалив существующие */
                for (size_t i = 0; i < size_; ++i) {
                    if (i < rhs.Size()) {
                        data_[i] = rhs.data_[i];
                    } else {
                        data_[i].~T();
                    }
                }
                if (rhs.Size() > size_) {
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.Size() - size_, data_.GetAddress() + size_);
                }
            }
            size_ = rhs.Size();
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        Swap(rhs);
        return *this;
    }
    
    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }
    
    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }
    
    void Reserve(size_t new_capacity);
    void Swap(Vector& other) noexcept;
    void Resize(size_t new_size);
    void PushBack(const T& value);
    void PushBack(T&& value);
    void PopBack() /* noexcept */;
    
    template <typename... Args>
    T& EmplaceBack(Args&&... args);
    
    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args);
    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/;
    iterator Insert(const_iterator pos, const T& value);
    iterator Insert(const_iterator pos, T&& value);

private:
    RawMemory<T> data_;
    size_t size_ = 0;
    
    void Uninitialized_Move_Or_Copy_N(iterator begin, size_t size, iterator new_begin);
};


template <typename T>
Vector<T>::Vector(size_t size)
    : data_(size)
    , size_(size) //
{
    std::uninitialized_value_construct_n(data_.GetAddress(), size);
}

template <typename T>
Vector<T>::~Vector() {
    std::destroy_n(data_.GetAddress(), size_);
}

template <typename T>
Vector<T>::Vector(const Vector& other)
    : data_(other.size_)
    , size_(other.size_) //
{
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
}

template <typename T>
Vector<T>::Vector(Vector&& other) noexcept
    : data_(0)
    , size_(0)
{
    Swap(other);
}

//Итераторы
template <typename T>
typename Vector<T>::iterator Vector<T>::begin() noexcept {
    return data_.GetAddress();
}

template <typename T>
typename Vector<T>::iterator Vector<T>::end() noexcept {
    return data_.GetAddress() + size_;
}

template <typename T>
typename Vector<T>::const_iterator Vector<T>::begin() const noexcept {
    return data_.GetAddress();
}

template <typename T>
typename Vector<T>::const_iterator Vector<T>::end() const noexcept {
    return data_.GetAddress() + size_;
}

template <typename T>
typename Vector<T>::const_iterator Vector<T>::cbegin() const noexcept {
    return begin();
}

template <typename T>
typename Vector<T>::const_iterator Vector<T>::cend() const noexcept {
    return end();
}

//методы
template <typename T>
void Vector<T>::Uninitialized_Move_Or_Copy_N(iterator begin, size_t size, iterator new_begin) {
    // constexpr оператор if будет вычислен во время компиляции
    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
        std::uninitialized_move_n(begin, size, new_begin);
    } else {
        std::uninitialized_copy_n(begin, size, new_begin);
    }
}

template <typename T>
void Vector<T>::Reserve(size_t new_capacity) {
    if (new_capacity <= data_.Capacity()) {
        return;
    }
    RawMemory<T> new_data(new_capacity);
    Uninitialized_Move_Or_Copy_N(data_.GetAddress(), size_, new_data.GetAddress());
    std::destroy_n(data_.GetAddress(), size_);
    data_.Swap(new_data);
}

template <typename T>
void Vector<T>::Swap(Vector& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
}

template <typename T>
void Vector<T>::Resize(size_t new_size) {
    Reserve(new_size);
    if (size_ > new_size) {
        std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
    } else if (size_ < new_size) {
        std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
    }
    size_ = new_size;
}

template <typename T>
void Vector<T>::PushBack(const T& value) {
    if (size_ == Capacity()) {
        if (size_ == 0) {
            Reserve(1);
            new (data_.GetAddress() + size_) T(value);
        } else {
            RawMemory<T> new_data(size_ * 2);
            new (new_data.GetAddress() + size_) T(value);
            Uninitialized_Move_Or_Copy_N(data_.GetAddress(), size_, new_data.GetAddress());
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
    } else {
        new (data_.GetAddress() + size_) T(value);
    }
    ++size_;
}

template <typename T>
void Vector<T>::PushBack(T&& value) {
    if (size_ == Capacity()) {
        if (size_ == 0) {
            Reserve(1);
            new (data_.GetAddress() + size_) T(std::move(value));
        } else {
            RawMemory<T> new_data(size_ * 2);
            new (new_data.GetAddress() + size_) T(std::move(value));
            Uninitialized_Move_Or_Copy_N(data_.GetAddress(), size_, new_data.GetAddress());
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
    } else {
        new (data_.GetAddress() + size_) T(std::move(value));
    }
    ++size_;
}

template <typename T>
void Vector<T>::PopBack() {
    if (size_ > 0){
        std::destroy_at(data_.GetAddress() + size_ - 1);
        --size_;
    }
}

template <typename T>
template <typename... Args>
T& Vector<T>::EmplaceBack(Args&&... args) {
    if (size_ == Capacity()) {
        if (size_ == 0) {
            Reserve(1);
            new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
        } else {
            RawMemory<T> new_data(size_ * 2);
            new (new_data.GetAddress() + size_) T(std::forward<Args>(args)...);
            Uninitialized_Move_Or_Copy_N(data_.GetAddress(), size_, new_data.GetAddress());
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
    } else {
        new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
    }
    ++size_;
    return data_[size_ - 1];
}

template <typename T>
template <typename... Args>
typename Vector<T>::iterator Vector<T>::Emplace(const_iterator pos, Args&&... args) {
    
    assert(pos >= begin() && pos <= end());
    
    auto elem_pos = begin();
    auto pos_num = pos - begin();
    
    if (size_ == Capacity()) {
        if (size_ == 0) {
            Reserve(1);
            elem_pos = new (end()) T(std::forward<Args>(args)...);
        } else {
            RawMemory<T> new_data(size_ * 2);
            elem_pos = new (new_data.GetAddress() + pos_num) T(std::forward<Args>(args)...);
        
            Uninitialized_Move_Or_Copy_N(begin(), pos_num, new_data.GetAddress());
            Uninitialized_Move_Or_Copy_N(begin() + pos_num, size_ - pos_num, new_data.GetAddress() + (pos_num + 1));
            
            std::destroy_n(begin(), size_);
            data_.Swap(new_data);
        }
    } else {
        if (size_ != 0) {
            T temp = T(std::forward<Args>(args)...);
            new (end()) T(std::forward<T>(*(end() - 1)));
            std::move_backward(begin() + pos_num, end() - 1, end());
            elem_pos = begin() + pos_num;
            *elem_pos = std::forward<T>(temp);
        } else {
            elem_pos = new (begin() + pos_num) T(std::forward<Args>(args)...);
        }
    }
    ++size_;
    return elem_pos;
}


template <typename T>
typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, const T& value) {
    return Emplace(pos, value);
}

template <typename T>
typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, T&& value) {
    return Emplace(pos, std::move(value));
}

template <typename T>
typename Vector<T>::iterator Vector<T>::Erase(const_iterator pos) {
    
    assert(pos >= begin() && pos <= end());
    
    auto elem_pos = begin() + (pos - begin());
    std::move(elem_pos + 1, end(), elem_pos);
    std::destroy_at(end() - 1);
    --size_;
    return elem_pos;
}


