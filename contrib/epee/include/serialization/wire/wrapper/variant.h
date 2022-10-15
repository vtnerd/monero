
#pragma once

namespace wire
{
  [[noreturn]] void throw_variant_exception(error::schema type, const char* variant_name);

  template<typename T, typename U>
  struct variant_option_
  {
    using variant_type = unwrap_reference_t<T>;
    using option_type = U;

    T variant;
    int last_id;

    [[noreturn]] static void throw_exception(const error::schema type)
    { throw_variant_exception(type, typeid(variant_type).name()); }

    variant_option_(T variant)
      : variant(std::move(variant)), last_id(current_id())
    {}

    variant_option_(variant_option&&) = default;
    variant_option_& operator==(variant_option_&&) = default;

    //! \throw wire::exception if two variant types were read (first type read was default).
    ~variant_option_()
    {
      if (!std::current_exception() && current_id() != last_id)
        throw_exception(error::invalid_key);
    }

    constexpr const variant_type& get_variant() const noexcept { return variant; }
    variant_type& get_variant() noexcept { return variant; }

    //! \return `true` iff `U` is active type in variant.
    bool is_active() const { return get<U>(std::addressof(get_variant())) != nullptr; }

    //! \return An integer indicating which type is active.
    int current_id() const noexcept { return get_variant().which(); }


    // concept requirements for optional fields

    //! \return `true` iff `U` is active type in variant.
    explicit operator bool() const { return is_active(); }

    //! \throw wire::exception iff another variant type has already been read.
    void emplace()
    {
      if (current_id() != last_id)
        throw_exception(error::schema::invalid_key);
      get_variant() = U{};
      last_id = current_id();
    }

    const U& operator*() const { return get<U>(get_variant()); }
    U& operator*() { return get<U>(get_variant()); }

    //! \throw wire::exception iff no variant type was read.
    void reset()
    {
      if (current_id() == last_id && is_active())
        throw_exception(error::schema::missing_key);
      last_id = current_id();
    }
  };

  template<typename U, typename T>
  inline constexpr variant_option_<T, U> variant_option(T variant)
  { return {std::move(variant)}; }
}
