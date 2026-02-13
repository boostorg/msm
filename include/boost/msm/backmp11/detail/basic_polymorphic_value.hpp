// Copyright 2026 Christian Granzin
// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_BACKMP11_DETAIL_BASIC_POLYMORPHIC_VALUE_HPP
#define BOOST_MSM_BACKMP11_DETAIL_BASIC_POLYMORPHIC_VALUE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

namespace boost::msm::backmp11::detail
{

// Basic polymorphic value with small buffer optimization.
// Similar to standard proposal P0201 (polymorphic_value).

struct control_block
{
    using move_fn_t = void(*)(void* dest, void* src) noexcept;
    using delete_fn_t = void(*)(void* obj) noexcept;

    inline void move(void*& dest, void*& src) const noexcept
    {
        if (is_inline)
        {
            // No move function implies trivially movable.
            if (!move_fn)
            {
                std::memcpy(&dest, &src, size);
            }
            else
            {
                move_fn(&dest, &src);
            }
        }
        else
        {
            dest = src;
            src = nullptr;
        }
    }

    inline void destroy(void* obj) const noexcept
    {
        if (delete_fn && obj)
        {
            delete_fn(obj);
        }
    }

    delete_fn_t delete_fn{nullptr};
    move_fn_t move_fn{nullptr};
    uint8_t size{0};
    bool is_inline{false};
};

template <typename T, bool IsInline>
struct create_control_block;
template <>
struct create_control_block<void, false>
{
    inline static const control_block instance{};
};
template <>
struct create_control_block<void, true>
{
    inline static const control_block instance{};
};
template <typename T>
struct create_control_block<T, /*IsInline=*/false>
{
    inline static const control_block instance{
        [](void* ptr) noexcept { delete static_cast<T*>(ptr); }};
};

template <typename T, bool IsTriviallyCopyable>
struct inline_control_bock
{
    inline static const control_block instance = []() constexpr {
        control_block self{};
        self.size = sizeof(T);
        self.is_inline = true;

        if constexpr (!std::is_trivially_move_constructible_v<T>)
        {
            self.move_fn = [](void* dest, void* src) noexcept {
                new (dest) T(std::move(*static_cast<T*>(src)));
            };
        }
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            self.delete_fn = [](void* ptr) noexcept {
                static_cast<T*>(ptr)->~T();
            };
        }

        return self;
    }();
};
template <typename T>
struct inline_control_bock<T, /*IsTriviallyCopyAble=*/true>
{
    inline static const control_block instance{nullptr, nullptr, sizeof(T), true};
};

template <typename T>
struct create_control_block<T, /*IsInline=*/true>
{
    inline static const control_block& instance =
        inline_control_bock<T, std::is_trivially_copyable_v<T>>::instance;
};

template <typename T, bool IsInline>
const control_block& control_block_v =
    create_control_block<T, IsInline>::instance;

struct inline_tag {};

template <size_t BufferSize, size_t BufferAlignment>
class basic_polymorphic_value_base
{
    static_assert(BufferSize <= 255,
                  "BufferSize must not be bigger than 255 Bytes");

  public:
    bool is_inline() const
    {
        return m_control_block->is_inline;
    }

  protected:
    explicit basic_polymorphic_value_base(const control_block& class_data)
        : m_control_block(&class_data)
    {
    }

    basic_polymorphic_value_base(const void* src, const control_block& class_data,
                        inline_tag)
        : m_control_block(&class_data)
    {
        std::memcpy(&this->m_buffer, src, m_control_block->size);
    }

    basic_polymorphic_value_base(void* ptr, control_block& class_data)
        : m_control_block(&class_data)
    {
        m_ptr = ptr;
    }

    ~basic_polymorphic_value_base()
    {
        m_control_block->destroy(get());
    }

    basic_polymorphic_value_base(const basic_polymorphic_value_base&) = delete;
    basic_polymorphic_value_base& operator=(const basic_polymorphic_value_base&) = delete;

    basic_polymorphic_value_base(basic_polymorphic_value_base&& other) noexcept
        : m_control_block(other.m_control_block)
    {
        m_control_block->move(m_ptr, other.m_ptr);
    }

    basic_polymorphic_value_base& operator=(basic_polymorphic_value_base&& other) noexcept
    {
        if (this != &other)
        {
            m_control_block->destroy(get());
            m_control_block = other.m_control_block;
            m_control_block->move(m_ptr, other.m_ptr);
        }
        return *this;
    }

    void* get() const noexcept
    {
        if (m_control_block->is_inline)
        {
            return const_cast<void*>(reinterpret_cast<const void*>(&m_buffer));
        }
        else
        {
            return m_ptr;
        }
    }

    const control_block* m_control_block{&control_block_v<void, true>};
    union {
        void* m_ptr;
        alignas(BufferAlignment) std::array<std::byte, BufferSize> m_buffer{};
    };
};

template <typename T,
          size_t BufferSize = 64 - sizeof(control_block*),
          size_t BufferAlignment = alignof(void*)>
class basic_polymorphic_value
    : public basic_polymorphic_value_base<BufferSize, BufferAlignment>
{
  private:
    using destroy_fn_t = void(*)(T* ptr) noexcept;
    using base = basic_polymorphic_value_base<BufferSize, BufferAlignment>;

    template <typename U>
    using IsInline = std::bool_constant<
        sizeof(U) <= BufferSize &&
        alignof(U) <= BufferAlignment>;

  public:
    basic_polymorphic_value(basic_polymorphic_value&& other) noexcept = default;
    basic_polymorphic_value& operator=(basic_polymorphic_value&& other) noexcept = default;

    template <typename U, bool IsInline = IsInline<U>::value>
    struct make_impl;
    template <typename U>
    struct make_impl<U, /*IsInline=*/true>
    {
        static_assert(std::is_base_of_v<T, U>, "U must be derived from T");

        static basic_polymorphic_value make(const U& obj)
        {
            if constexpr (std::is_trivially_copyable_v<U>)
            {
                return basic_polymorphic_value{&obj, control_block_v<U, true>,
                                      inline_tag{}};
            }
            else
            {
                basic_polymorphic_value self{control_block_v<U, true>};
                new (&self.m_buffer) U(obj);
                return self;
            }
        }

        template <typename... Args>
        static basic_polymorphic_value make(Args&&... args)
        {
            basic_polymorphic_value self{control_block_v<U, true>};
            new (&self.m_buffer) U(std::forward<Args>(args)...);
            return self;
        }
    };
    template <typename U>
    struct make_impl<U, /*IsInline=*/false>
    {
        static_assert(std::is_base_of_v<T, U>, "U must be derived from T");

        template <typename... Args>
        static basic_polymorphic_value make(Args&&... args)
        {
            basic_polymorphic_value self{control_block_v<U, false>};
            self.m_ptr = new U(std::forward<Args>(args)...);
            return self;
        }
    };

    template <typename U, typename... Args>
    static basic_polymorphic_value make(Args&&... args)
    {
        return make_impl<U>::make(std::forward<Args>(args)...);
    };

    template <typename U>
    static constexpr basic_polymorphic_value make(const U& obj)
    {
        return make_impl<U>::make(obj);
    };

    T* get() const noexcept
    {
        return static_cast<T*>(base::get());
    }

    T& operator*() const noexcept
    {
        return *get();
    }

    T* operator->() const noexcept
    {
        return get();
    }

  protected:
    using base::base;
};

} // namespace boost::msm::backmp11::detail

#endif // BOOST_MSM_BACKMP11_DETAIL_BASIC_POLYMORPHIC_VALUE_HPP
