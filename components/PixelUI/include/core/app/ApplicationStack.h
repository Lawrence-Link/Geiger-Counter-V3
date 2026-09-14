/*
 * Copyright (C) 2025 Lawrence Link
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "IApplication.h"
#include "config.h"

#include <etl/placement_new.h>
#include <etl/type_traits.h>
#include <etl/utility.h>

static_assert(APPLICATION_ARENA_SIZE >= APPLICATION_ARENA_MAX_ALIGNMENT,
              "Application arena must be at least as large as its maximum alignment");
static_assert((APPLICATION_ARENA_MAX_ALIGNMENT & (APPLICATION_ARENA_MAX_ALIGNMENT - 1U)) == 0U,
              "APPLICATION_ARENA_MAX_ALIGNMENT must be a power of two");

enum class ApplicationStackResult {
    Ok,
    StackFull,
    ArenaFull,
    ConstructionFailed,
};

class ApplicationFactory {
    friend class ApplicationStack;

public:
    using ConstructFunction = IApplication* (*)(void* storage, PixelUI& ui, void* parameters);
    using DestroyFunction = void (*)(IApplication* application);

    /** @brief Creates an empty factory with no construction callbacks. */
    constexpr ApplicationFactory() = default;

    template <typename T>
    /**
     * @brief Creates a factory for constructing and destroying T in caller storage.
     * @tparam T Concrete IApplication type.
     */
    static constexpr ApplicationFactory make() {
        validateType<T>();
        return ApplicationFactory{
            sizeof(T),
            alignof(T),
            &constructDefault<T>,
            &destroyConcrete<T>,
        };
    }

    template <typename T>
    /**
     * @brief Creates a factory with a custom construction callback.
     * @tparam T Concrete IApplication type to destroy.
     * @param construct Callback that constructs T in supplied storage.
     */
    static constexpr ApplicationFactory makeCustom(ConstructFunction construct) {
        validateType<T>();
        return ApplicationFactory{sizeof(T), alignof(T), construct, &destroyConcrete<T>};
    }

private:
    /** @brief Stores factory metadata and construction callbacks. */
    constexpr ApplicationFactory(
        size_t objectSize,
        size_t objectAlignment,
        ConstructFunction construct,
        DestroyFunction destroy)
        : objectSize_(objectSize), objectAlignment_(objectAlignment),
          construct_(construct), destroy_(destroy) {}

    template <typename T>
    /** @brief Enforces the arena and inheritance requirements for T. */
    static constexpr void validateType() {
        static_assert(etl::is_base_of<IApplication, T>::value,
                      "Application type must derive from IApplication");
        static_assert((alignof(T) & (alignof(T) - 1U)) == 0U,
                      "Application alignment must be a power of two");
        static_assert(alignof(T) <= APPLICATION_ARENA_MAX_ALIGNMENT,
                      "Application alignment exceeds APPLICATION_ARENA_MAX_ALIGNMENT");
        static_assert(sizeof(T) <= APPLICATION_ARENA_SIZE,
                      "Application is larger than APPLICATION_ARENA_SIZE");
    }

    template <typename T>
    /** @brief Constructs T with a PixelUI reference and parameter pointer. */
    static IApplication* constructDefault(void* storage, PixelUI& ui, void* parameters) {
        return ::new (storage) T(ui, parameters);
    }

    template <typename T>
    /** @brief Invokes the concrete destructor for an application pointer. */
    static void destroyConcrete(IApplication* application) {
        static_cast<T*>(application)->~T();
    }

    size_t objectSize_ = 0;
    size_t objectAlignment_ = 0;
    ConstructFunction construct_ = nullptr;
    DestroyFunction destroy_ = nullptr;
};

class ApplicationStack {
public:
    /** @brief Creates an empty fixed-capacity application stack. */
    ApplicationStack() = default;
    /** @brief Destroys all applications remaining on the stack. */
    ~ApplicationStack() { clear(); }

    /** @brief Copy construction is disabled because the stack owns arena objects. */
    ApplicationStack(const ApplicationStack&) = delete;
    /** @brief Copy assignment is disabled because the stack owns arena objects. */
    ApplicationStack& operator=(const ApplicationStack&) = delete;

    template <typename T, typename... Args>
    /**
     * @brief Constructs T in the fixed arena and pushes it onto the stack.
     * @param application Receives the constructed application, or nullptr on failure.
     * @param args Arguments forwarded to T's constructor.
     * @return Result describing success or capacity failure.
     */
    ApplicationStackResult emplace(T*& application, Args&&... args) {
        static_assert(etl::is_base_of<IApplication, T>::value,
                      "Application type must derive from IApplication");
        static_assert((alignof(T) & (alignof(T) - 1U)) == 0U,
                      "Application alignment must be a power of two");
        static_assert(alignof(T) <= APPLICATION_ARENA_MAX_ALIGNMENT,
                      "Application alignment exceeds APPLICATION_ARENA_MAX_ALIGNMENT");
        static_assert(sizeof(T) <= APPLICATION_ARENA_SIZE,
                      "Application is larger than APPLICATION_ARENA_SIZE");

        Layout layout{};
        const ApplicationStackResult layoutResult = prepareLayout(sizeof(T), alignof(T), layout);
        if (layoutResult != ApplicationStackResult::Ok) {
            application = nullptr;
            return layoutResult;
        }

        T* constructed =
            ::new (arena_ + layout.alignedOffset) T(etl::forward<Args>(args)...);

        commit(constructed, layout, &destroyConcrete<T>);
        application = constructed;
        return ApplicationStackResult::Ok;
    }

    /**
     * @brief Constructs an application through an ApplicationFactory.
     * @param factory Factory containing construction and destruction callbacks.
     * @param ui UI instance passed to the factory.
     * @param parameters Optional parameter passed to the factory.
     * @param application Receives the constructed application, or nullptr on failure.
     * @return Result describing success or construction/capacity failure.
     */
    ApplicationStackResult emplace(
        const ApplicationFactory& factory,
        PixelUI& ui,
        void* parameters,
        IApplication*& application);

    /** @brief Destroys and removes the application at the top of the stack. */
    void pop();
    /** @brief Pops applications until the stack is empty. */
    void clear();

    /** @return The top application, or nullptr when the stack is empty. */
    IApplication* top() const noexcept;
    /** @return The application below the top, or nullptr when fewer than two exist. */
    IApplication* previous() const noexcept;
    /** @return Whether no applications are currently stored. */
    bool empty() const noexcept { return depth_ == 0U; }
    /** @return Whether the fixed application depth has been reached. */
    bool full() const noexcept { return depth_ == MAX_VIEW_DEPTH; }
    /** @return Number of applications currently stored. */
    size_t depth() const noexcept { return depth_; }
    /** @return Number of arena bytes occupied by stored applications. */
    size_t used() const noexcept { return offset_; }
    /** @return Total number of bytes available in the application arena. */
    static constexpr size_t capacity() noexcept { return APPLICATION_ARENA_SIZE; }

private:
    using DestroyFunction = ApplicationFactory::DestroyFunction;

    struct Entry {
        IApplication* application = nullptr;
        size_t marker = 0;
        size_t endOffset = 0;
        DestroyFunction destroy = nullptr;
    };

    struct Layout {
        size_t marker = 0;
        size_t alignedOffset = 0;
        size_t endOffset = 0;
    };

    /** @brief Validates stack and arena space and computes aligned object offsets. */
    ApplicationStackResult prepareLayout(size_t objectSize, size_t alignment, Layout& layout) const;
    /** @brief Commits a constructed object and advances stack and arena state. */
    void commit(IApplication* application, const Layout& layout, DestroyFunction destroy);

    template <typename T>
    /** @brief Invokes the concrete destructor for an application pointer. */
    static void destroyConcrete(IApplication* application) {
        static_cast<T*>(application)->~T();
    }

    alignas(APPLICATION_ARENA_MAX_ALIGNMENT) uint8_t arena_[APPLICATION_ARENA_SIZE]{};
    Entry entries_[MAX_VIEW_DEPTH]{};
    size_t offset_ = 0;
    size_t depth_ = 0;
};
