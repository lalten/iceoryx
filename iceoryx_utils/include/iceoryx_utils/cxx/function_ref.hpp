// Copyright (c) 2020 by Robert Bosch GmbH. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <memory>
#include <type_traits>

namespace iox
{
namespace cxx
{
/// @brief cxx::function_ref is a lightweight alternative to std::function
///
///        It is a non-owning reference to a callable.
///
///        It shall have these features:
///         * No heap usage
///         * No exceptions
///         * Stateful lambda support
///         * C++11/14 support
/// @code
///         // Usage as function parameter
///         void fuu(cxx::function_ref<void()> callback)
///         {
///             callback();
///         }
///         // Call the lambda
///         fuu([]{ doSomething(); });
///
///         // Usage with l-values
///         // Pitfall: Ensure that lifetime of callable suits the point in time of calling callback()
///         auto callable = [&]{ doSomething(); };
///         cxx::function_ref<void()> callback(callable);
///         // Call the callback
///         callback();
/// @endcode

template <typename SignatureType>
class function_ref;

template <class ReturnType, class... ArgTypes>
class function_ref<ReturnType(ArgTypes...)>
{
    using SignatureType = ReturnType(ArgTypes...);

    template <typename CallableType>
    using EnableIfNotFunctionRef = std::enable_if<!std::is_same<std::decay<CallableType>, function_ref>::value>;

  public:
    /// @todo Remove empty instantiation , if you want to have it empty, use an cxx::optional around cxx::function_ref
    /// @brief Creates an empty function_ref
    function_ref() noexcept
        : m_target(nullptr)
        , m_functionPointer(nullptr)
    {
    }

    /// @brief Creates an empty function_ref
    function_ref(nullptr_t) noexcept
        : m_target(nullptr)
        , m_functionPointer(nullptr)
    {
    }

    /// @brief D'tor
    ~function_ref() noexcept = default;

    /// @brief No copy c'tor
    /// @todo test this
    function_ref(const function_ref&) noexcept = delete;

    /// @brief No copy assignment
    function_ref& operator=(const function_ref&) noexcept = default;

    /// @brief Create a function_ref
    /// @todo Type trait std::is_invocable is only available in C++17, workaround for C++11/14?
    template <typename CallableType, typename = EnableIfNotFunctionRef<CallableType>>
    function_ref(CallableType&& callable) noexcept
        : m_target(reinterpret_cast<void*>(std::addressof(callable)))
    {
        m_functionPointer = [](void* target, ArgTypes... args) -> ReturnType {
            return (*reinterpret_cast<typename std::add_pointer<CallableType>::type>(target))(
                std::forward<ArgTypes>(args)...);
        };
    }

    /// @brief Move assignment
    function_ref& operator=(function_ref&& rhs) noexcept = default;

    /// @brief Calls the provided callable
    auto operator()(ArgTypes... args) const noexcept -> ReturnType
    {
        if (!m_target)
        {
            // Callable was called without user having assigned one beforehand
            std::terminate();
        }
        return m_functionPointer(m_target, std::forward<ArgTypes>(args)...);
    }

    /// @brief Checks whether a valid target is contained
    explicit operator bool() const noexcept
    {
        return m_target != nullptr;
    }

    void swap(function_ref<SignatureType>& rhs) noexcept
    {
        std::swap(m_target, rhs.m_target);
        std::swap(m_functionPointer, rhs.m_functionPointer);
    }

    /// @todo Be closer to std::function API
    // target() ?
    // target_type() ?

  private:
    /// @brief Raw pointer of the callable
    void* m_target{nullptr};

    /// @brief Function pointer to the callable
    ReturnType (*m_functionPointer)(void*, ArgTypes...){nullptr};
};

template <class ReturnType, class... ArgTypes>
void swap(function_ref<ReturnType(ArgTypes...)>& lhs, function_ref<ReturnType(ArgTypes...)>& rhs) noexcept
{
    lhs.swap(rhs);
}

} // namespace cxx
} // namespace iox
