#pragma once

#include "property_details.h"
#include "raii.h"
#include <string>
#include <string_view>

namespace prop {
	using detail::converts_to;

	template <class T, bool with_initial_value>
	struct Generator_data;

	template <class T>
	struct Generator_data<T, true> {
		Generator_data(prop::detail::Generator_function<T> auto &&f)
			: value{f()}
			, source{prop::detail::make_direct_update_function(std::forward<decltype(f)>(f))} {}
		template <class... Properties>
		Generator_data(prop::detail::Generator_function<T, Properties...> auto &&f, Properties &&...properties) {
			detail::Property_function_binder<T> binder{std::forward<decltype(f)>(f),
													   std::forward<Properties>(properties)...};
			source = std::move(binder.function);
			dependencies = std::move(binder.dependencies);
		}
		T value;
		std::move_only_function<prop::Updater_result(prop::Property<T> &,
													 std::span<const prop::Property_link::Property_pointer>)>
			source;
		std::vector<prop::Property_link> dependencies;
	};

	template <class T>
	struct Generator_data<T, false> {
		Generator_data(prop::detail::Generator_function<T> auto &&f)
			: source{prop::detail::make_direct_update_function(std::forward<decltype(f)>(f))} {}
		template <class... Properties>
		Generator_data(prop::detail::Generator_function<T, Properties...> auto &&f, Properties &&...properties) {
			detail::Property_function_binder<T> binder{std::forward<decltype(f)>(f),
													   std::forward<Properties>(properties)...};
			source = std::move(binder.function);
			dependencies = std::move(binder.dependencies);
		}
		std::move_only_function<prop::Updater_result(prop::Property<T> &,
													 std::span<const prop::Property_link::Property_pointer>)>
			source;
		std::vector<prop::Property_link> dependencies;
	};

	template <class T, bool with_initial_value>
	struct Generator {
		Generator_data<T, with_initial_value> data;
	};

	template <class T>
	struct Updater_data {
		template <class F>
		Updater_data(F &&f)
			: source{prop::detail::make_direct_update_function(std::forward<F>(f))} {}
		template <class F, class... Properties>
		Updater_data(F &&f, Properties &&...properties) {
			detail::Property_function_binder<T> binder{std::forward<F>(f), std::forward<Properties>(properties)...};
			source = std::move(binder.function);
			dependencies = std::move(binder.dependencies);
		}
		std::move_only_function<prop::Updater_result(prop::Property<T> &,
													 std::span<const prop::Property_link::Property_pointer>)>
			source;
		std::vector<prop::Property_link> dependencies;
	};

	template <class T>
	struct Updater {
		Updater_data<T> data;
	};

	namespace detail {
		template <class T>
		concept Streamable = requires(std::ostream &os, const T &t) { os << t; };
		template <class T>
		struct Printer {
			Printer(const T &t)
				: value{t} {}
			const T &value;
			std::ostream &print(std::ostream &os) {
				if constexpr (Streamable<T>) {
					return os << value;
				} else {
					return os << prop::type_name<T>() << '@' << &value;
				}
			}
		};
		template <class T>
		Printer(const T &) -> Printer<T>;

		template <class T>
		std::ostream &operator<<(std::ostream &os, prop::detail::Printer<T> &&printer) {
			return printer.print(os);
		}
	} // namespace detail

	template <class T>
	class Property : public prop::Property_link {
		mutable T value{};
		std::move_only_function<prop::Updater_result(prop::Property<T> &,
													 std::span<const prop::Property_link::Property_pointer>)>
			source;
		struct Write_notifier;

		public:
		using Value_type = T;
		struct Value {
			T value;
		};

		std::string_view type() const override {
			return prop::type_name<prop::Property<T>>();
		}

		std::string value_string() const override {
			std::stringstream ss;
			ss << prop::detail::Printer(value);
			return std::move(ss).str();
		}
		bool has_source() const override {
			return false;
		}

		public:
		Property();
		Property(const Property<T> &other);
		Property(Property<T> &&other);
		template <class... Args>
		Property(Args &&...args)
			requires std::constructible_from<T, Args &&...>;
		Property(Value &&v);
		Property(Generator<T, true> &&generator);
		Property(prop::detail::Generator_function<T> auto &&f);
		Property(prop::detail::Updater_function<T> auto &&f, T t = {});
		Property(prop::detail::Property_function_binder<T> binder);
		virtual ~Property() {
			//prevents infinite recursion when source is a lambda that captures a property by value and depends on it
			auto{std::move(source)};
		}

		Property<T> &operator=(const T &t)
			requires std::assignable_from<T &, const T &>
		{
			unbind();
			value = t;
			write_notify();
			return *this;
		}
		Property<T> &operator=(T &&t)
			requires std::assignable_from<T &, T &&>
		{
			unbind();
			value = std::move(t);
			write_notify();
			return *this;
		}
		Property<T> &operator=(Property<T> &&other) {
			value = std::move(other.value);
			source = std::move(other.source);
			static_cast<prop::Property_link &>(*this) = static_cast<prop::Property_link &&>(other);
			return *this;
		}
		Property<T> &operator=(const Property<T> &other) {
			unbind();
			value = other.value;
			write_notify();
			return *this;
		}
		template <class U>
		Property &operator=(U &&u)
			requires std::is_assignable_v<T &, U &&>;
		Property &operator=(Value &&v);
		Property &operator=(Generator<T, false> &&generator);
		Property &operator=(Updater<T> &&updater);
		Property &operator=(prop::detail::Updater_function<T> auto &&updater);
		Property &operator=(prop::detail::Generator_function<T> auto &&generator);
		Property &operator=(prop::detail::Property_function_binder<T> binder);

		void set(T t);
		const T &get() const;
		bool is_bound() const;
		void unbind() final override;
		std::string displayed_value() const final {
			return prop::to_display_string(value, 30);
		}
		void sever();
		Write_notifier apply();
		template <class Functor>
		std::invoke_result_t<Functor, T &> apply(Functor &&f);

		template <class U>
		bool is_implicit_dependency_of(const Property<U> &other) const;
		template <class U>
		bool is_explicit_dependency_of(const Property<U> &other) const;
		template <class U>
		bool is_dependency_of(const Property<U> &other) const;
		template <class U>
		bool is_implicit_dependent_of(const Property<U> &other) const;
		template <class U>
		bool is_explicit_dependent_of(const Property<U> &other) const;
		template <class U>
		bool is_dependent_on(const Property<U> &other) const;
		template <class U>
		operator U &()
			requires(prop::detail::Conversion_type<T, U>::pass_by_ref)
		{
			read_notify();
			return value;
		}
		template <class U>
		operator U &&()
			requires(prop::detail::Conversion_type<T, U>::pass_by_temp_ref)
		{
			read_notify();
			return value;
		}
		template <class U>
		operator U()
			requires(prop::detail::Conversion_type<T, U>::pass_by_value)
		{
			read_notify();
			return value;
		}
		template <class U>
		operator const U()
			requires(prop::detail::Conversion_type<T, U>::pass_by_const_value)
		{
			read_notify();
			return value;
		}
		operator const T &() const;
		auto operator->() const {
			read_notify();
			if constexpr (std::is_pointer_v<T>) {
				return value;
			} else {
				return &value;
			}
		}
		const T &operator*() const {
			read_notify();
			return value;
		}

#define PROP_OP(OP)                                                                                                    \
	decltype(auto) operator OP() const                                                                                 \
		requires(requires { OP value; })                                                                               \
	{                                                                                                                  \
		read_notify();                                                                                                 \
		return OP value;                                                                                               \
	}                                                                                                                  \
	decltype(auto) operator OP()                                                                                       \
		requires(                                                                                                      \
			not requires { OP std::as_const(value); } and requires { OP value; })                                      \
	{                                                                                                                  \
		Write_notifier wn{this};                                                                                       \
		return OP value;                                                                                               \
	}

		//Unary ops: + - * & ~ ! ++ --
		PROP_OP(+)
		PROP_OP(-)
		PROP_OP(*)
		//PROP_OP(&) //don't forward, taking address of property should work as normal
		PROP_OP(~)
		PROP_OP(!)
		PROP_OP(++)
		PROP_OP(--)
#undef PROP_OP

		decltype(auto) operator++(int) const
			requires(requires { value++; })
		{
			read_notify();
			return value++;
		}
		decltype(auto) operator++(int)
			requires(
				not requires { std::as_const(value)++; } and requires { value++; })
		{
			Write_notifier wn{this};
			return value++;
		}

		decltype(auto) operator--(int) const
			requires(requires { value--; })
		{
			read_notify();
			return value--;
		}
		decltype(auto) operator--(int)
			requires(
				not requires { std::as_const(value)--; } and requires { value--; })
		{
			Write_notifier wn{this};
			return value--;
		}

#if 0 //This ICEs gcc 15, try again with gcc 16
		template <class U>
		decltype(auto) operator,(U &&u) const
			requires(requires { value, std::forward<U>(u); })
		{
			read_notify();
			return value, std::forward<U>(u);
		}
		template <class U>
		decltype(auto) operator,(U &&u)
			requires(
				not requires { std::as_const(value), std::forward<U>(u); } and requires { value, std::forward<U>(u); })
		{
			Write_notifier wn{this};
			return value, std::forward<U>(u);
		}
#endif

		template <class... Args>
		decltype(auto) operator()(Args &&...args) const
			requires(requires { value(std::forward<Args>(args)...); })
		{
			read_notify();
			return value(std::forward<Args>(args)...);
		}
		template <class... Args>
		decltype(auto) operator()(Args &&...args)
			requires(
				not requires { std::as_const(value)(std::forward<Args>(args)...); } and
				requires { value(std::forward<Args>(args)...); })
		{
			Write_notifier wn{this};
			return value(std::forward<Args>(args)...);
		}

		decltype(auto) operator[](std::convertible_to<std::size_t> auto &&u) const
			requires(requires { value[std::forward<decltype(u)>(static_cast<std::size_t>(u))]; })
		{
			read_notify();
			return value[std::forward<decltype(u)>(u)];
		}
		decltype(auto) operator[](std::convertible_to<std::size_t> auto &&u)
			requires(
				not requires { std::as_const(value)[std::forward<decltype(u)>(u)]; } and
				requires { value[std::forward<decltype(u)>(static_cast<std::size_t>(u))]; })
		{
			Write_notifier wn{this};
			return value[std::forward<decltype(u)>(u)];
		}

		template <class... Args>
		decltype(auto) operator[](Args &&...args) const
			requires(requires { value[std::forward<Args>(args)...]; } and sizeof...(Args) != 1)
		{
			read_notify();
			return value[std::forward<Args>(args)...];
		}
		template <class... Args>
		decltype(auto) operator[](Args &&...args)
			requires(
				requires { value[std::forward<Args>(args)...]; } and
				not requires { std::as_const(value)[std::forward<Args>(args)...]; } and sizeof...(Args) != 1)
		{
			read_notify();
			return value[std::forward<Args>(args)...];
		}

#ifdef PROPERTY_NAMES
		using prop::Property_link::custom_name;
#endif

		private:
		struct Write_notifier {
			decltype(auto) operator->() && {
				if constexpr (prop::detail::has_operator_arrow_v<decltype(p->value)>) {
					return static_cast<decltype(p->value) &>(p->value);
				} else {
					return &p->value;
				}
			}
			~Write_notifier() {
				if (p) {
					p->write_notify();
				}
			}

			private:
			Write_notifier(prop::Property<T> *p_)
				: p{p_} {}
			friend class prop::Property<T>;
			prop::Property<T> *p;
		};
		void update_source(std::move_only_function<prop::Updater_result(
							   prop::Property<T> &, std::span<const prop::Property_link::Property_pointer>)>
							   f);
		void update() override final;
		friend class Binding;

		template <class U>
		friend class Property;
		friend class prop::Property_link;
		template <class U>
		friend std::move_only_function<prop::Updater_result(prop::Property<U> &,
															std::span<const prop::Property_link::Property_pointer>)>
		prop::detail::make_direct_update_function(prop::detail::Generator_function<U> auto &&f);
		template <class U>
		std::move_only_function<prop::Updater_result(prop::Property<U> &,
													 std::span<const prop::Property_link::Property_pointer>)>
		make_direct_update_function(prop::detail::Property_update_function<U> auto &&f);
	};

	template <>
	class Property<void> : public prop::Property_link {
		public:
		using prop::Property_link::print_status;
		using Value_type = void;
		Property();
		Property(Property &&other);
		Property &operator=(Property &&other);
		Property(std::convertible_to<std::move_only_function<void()>> auto &&f)
			: source{prop::detail::create_explicit_caller<void>(std::forward<decltype(f)>(f), {})} {}
		Property &operator=(std::convertible_to<std::move_only_function<void()>> auto &&f);
		Property &operator=(prop::detail::Property_function_binder<void> binder);
		bool is_bound() const;
		void unbind() final override;
		std::string displayed_value() const final {
			return "<void>";
		}
		std::string_view type() const override {
			return "void";
		}
		std::string value_string() const override {
			return "void";
		}
		bool has_source() const override {
			return !!source;
		}

		template <class U>
		bool is_implicit_dependency_of(const Property<U> &other) const;
		template <class U>
		bool is_explicit_dependency_of(const Property<U> &other) const;
		template <class U>
		bool is_dependency_of(const Property<U> &other) const;
		template <class U>
		bool is_implicit_dependent_of(const Property<U> &other) const;
		template <class U>
		bool is_explicit_dependent_of(const Property<U> &other) const;
		template <class U>
		bool is_dependent_on(const Property<U> &other) const;
#ifdef PROPERTY_NAMES
		using prop::Property_link::custom_name;
#endif
		virtual ~Property() = default;

		private:
		void update_source(std::move_only_function<void(std::span<const prop::Property_link::Property_pointer>)> f);
		void update() override final;
		std::move_only_function<void(std::span<const prop::Property_link::Property_pointer>)> source;
		friend class Binding;
		template <class U>
		friend class Property;
	};

	template <class U>
	bool Property<void>::is_implicit_dependency_of(const Property<U> &other) const {
		return prop::Property_link::is_implicit_dependency_of(other);
	}

	template <class U>
	bool Property<void>::is_explicit_dependency_of(const Property<U> &other) const {
		return prop::Property_link::is_explicit_dependency_of(other);
	}

	template <class U>
	bool Property<void>::is_dependency_of(const Property<U> &other) const {
		return prop::Property_link::is_dependency_of(other);
	}

	template <class U>
	bool Property<void>::is_implicit_dependent_of(const Property<U> &other) const {
		return prop::Property_link::is_implicit_dependent_of(other);
	}

	template <class U>
	bool Property<void>::is_explicit_dependent_of(const Property<U> &other) const {
		return prop::Property_link::is_explicit_dependent_of(other);
	}

	template <class U>
	bool Property<void>::is_dependent_on(const Property<U> &other) const {
		return prop::Property_link::is_dependent_on(other);
	}

	namespace detail {
		template <class T>
		T inner_value_type(prop::Property<T> &&);
		template <class T>
		T inner_value_type(T &&t);

		template <class T>
		using inner_value_type_t =
			std::remove_cvref_t<decltype(prop::detail::inner_value_type(std::declval<std::remove_cvref_t<T>>()))>;

		template <class T>
		using inner_function_type_t = inner_value_type_t<decltype(std::declval<T>()())>;

		template <class T>
		inner_function_type_t<T> inner_type(std::true_type);
		template <class T>
		inner_value_type_t<T> inner_type(std::false_type);

		template <class T>
		auto is_function_type(T &&) -> decltype(std::declval<T>()(), void(0), std::true_type{});
		std::false_type is_function_type(...);

		template <class T>
		constexpr bool is_function_type_v = decltype(is_function_type(std::declval<T>()))::value;

		template <class T>
		using inner_type_t = decltype(inner_type<T>(is_function_type(std::declval<T>())));
	} // namespace detail

	template <class T>
	Property<T>::Property()
		:
#ifdef PROPERTY_NAMES
		Property_link(prop::type_name<prop::Property<T>>())
		,
#endif
		value{} {
	}

	template <class T>
	Property<T>::Property(const Property<T> &other)
		:
#ifdef PROPERTY_NAMES
		Property_link(prop::type_name<prop::Property<T>>())
		,
#endif
		value{other.value} {
		other.read_notify();
	}
	template <class T>
	Property<T>::Property(Property<T> &&other)
		:
#ifdef PROPERTY_NAMES
		Property_link(prop::type_name<prop::Property<T>>())
		,
#endif
		value{std::move(other.value)}
		, source{std::move(other.source)} {
		static_cast<prop::Property_link &>(*this) = static_cast<prop::Property_link &&>(other);
	}

	template <class T>
	Property<T>::Property(prop::detail::Property_function_binder<T> binder)
#ifdef PROPERTY_NAMES
		: Property_link(prop::type_name<prop::Property<T>>())
#endif
	{
		*this = std::move(binder);
	} // namespace prop

	template <class T>
	Property<T>::Property(prop::detail::Generator_function<T> auto &&f)
		:
#ifdef PROPERTY_NAMES
		Property_link(prop::type_name<prop::Property<T>>())
		,
#endif
		value{[&] {
			if constexpr (std::is_default_constructible_v<T>) {
				return T{};
			} else {
				return f();
			}
		}()}
		, source{prop::detail::make_direct_update_function<T>(std::forward<decltype(f)>(f))} {
		if constexpr (std::is_default_constructible_v<T>) {
			update();
		}
	}

	template <class T>
	Property<T>::Property(prop::detail::Updater_function<T> auto &&f, T t)
		:
#ifdef PROPERTY_NAMES
		Property_link(prop::type_name<prop::Property<T>>())
		,
#endif
		value{std::move(t)} {
		update_source(prop::detail::make_direct_update_function<T>(std::forward<decltype(f)>(f)));
	}

	template <class T>
	template <class... Args>
	Property<T>::Property(Args &&...args)
		requires std::constructible_from<T, Args &&...>
		:
#ifdef PROPERTY_NAMES
		Property_link(prop::type_name<prop::Property<T>>())
		,
#endif
		value{std::forward<Args>(args)...} {
	}

	template <class T>
	Property<T>::Property(Value &&v)
		:
#ifdef PROPERTY_NAMES
		Property_link(prop::type_name<prop::Property<T>>())
		,
#endif
		value{std::move(v.value)} {
	}

	template <class T>
	Property<T>::Property(Generator<T, true> &&generator)
		:
#ifdef PROPERTY_NAMES
		Property_link(prop::type_name<prop::Property<T>>())
		,
#endif
		value{std::move(generator.generator.value)} {
		update_source(std::move(generator.generator.source));
	}

	template <class T>
	template <class U>
	Property<T> &Property<T>::operator=(U &&u)
		requires std::is_assignable_v<T &, U &&>
	{
		value = std::forward<U>(u);
		write_notify();
		return *this;
	}

	template <class T>
	Property<T> &Property<T>::operator=(Generator<T, false> &&generator) {
		unbind();
		set_explicit_dependencies(std::move(generator.data.dependencies));
		update_source(std::move(generator.data.source));
		return *this;
	}

	template <class T>
	Property<T> &Property<T>::operator=(prop::detail::Generator_function<T> auto &&f) {
		return *this = prop::detail::Property_function_binder<T>{std::forward<decltype(f)>(f)};
	}
	template <class T>
	Property<T> &Property<T>::operator=(prop::detail::Updater_function<T> auto &&f) {
		return *this = prop::detail::Property_function_binder<T>{std::forward<decltype(f)>(f)};
	}
	template <class T>
	Property<T> &Property<T>::operator=(prop::detail::Property_function_binder<T> binder) {
		prop::Property_link::set_explicit_dependencies(std::move(binder.dependencies));
		update_source(std::move(binder.function));
		return *this;
	}

	template <class T>
	Property<T>::operator const T &() const {
		return get();
	}

	template <class T>
	void Property<T>::set(T t) {
		unbind();
		if (!detail::is_equal(value, t)) {
			Write_notifier wn{this};
			value = std::move(t);
		}
	}

	template <class T>
	const T &Property<T>::get() const {
		read_notify();
		return value;
	}

	template <class T>
	bool Property<T>::is_bound() const {
		return source != nullptr;
	}

	template <class T>
	void Property<T>::unbind() {
		source = nullptr;
		prop::Property_link::unbind();
	}

	template <class T>
	Property<T>::Write_notifier Property<T>::apply() {
		return {this};
	}

	template <class T>
	template <class Functor>
	std::invoke_result_t<Functor, T &> Property<T>::apply(Functor &&f) {
		Write_notifier wn{this};
		read_notify();
		if constexpr (std::is_fundamental_v<T> || std::is_pointer_v<T>) {
			T t = value;
			try {
				auto retval = f(value);
				if (t == value) {
					wn.p = nullptr;
				}
				return retval;
			} catch (...) {
				if (t == value) {
					wn.p = nullptr;
				}
				throw;
			}
		} else {
			return f(value);
		}
	}

	template <class T>
	template <class U>
	bool Property<T>::is_implicit_dependency_of(const Property<U> &other) const {
		return prop::Property_link::is_implicit_dependency_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_explicit_dependency_of(const Property<U> &other) const {
		return prop::Property_link::is_explicit_dependency_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_dependency_of(const Property<U> &other) const {
		return prop::Property_link::is_dependency_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_implicit_dependent_of(const Property<U> &other) const {
		return prop::Property_link::is_implicit_dependent_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_explicit_dependent_of(const Property<U> &other) const {
		return prop::Property_link::is_explicit_dependent_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_dependent_on(const Property<U> &other) const {
		return prop::Property_link::is_dependent_on(other);
	}

	template <class T>
	void Property<T>::update_source(std::move_only_function<prop::Updater_result(
										prop::Property<T> &, std::span<const prop::Property_link::Property_pointer>)>
										f) {
		std::swap(f, source);
		update();
	}

	template <class T>
	void Property<T>::update() {
		prop::Property_link *previous_binding;
		prop::detail::RAII updater{[this, &previous_binding] { update_start(previous_binding); },
								   [this, &previous_binding] { update_complete(previous_binding); }};
		try {
			switch (source(*this, this->get_explicit_dependencies())) {
				case prop::Updater_result::sever:
					unbind();
					[[fallthrough]];
				case prop::Updater_result::changed:
					updater.early_exit();
					write_notify();
					break;
				case prop::Updater_result::unchanged:
					updater.early_exit();
					break;
			}
		} catch (...) {
			//on_property_update_exception(this, std::current_exception());
			unbind();
			throw;
		}
#if 0
		Write_notifier wn{this};
		auto call_source = [this, &previous_binding] {
			prop::detail::RAII updater{[this, &previous_binding] { update_start(previous_binding); },
									   [this, &previous_binding] { update_complete(previous_binding); }};
			return source(explicit_dependencies);
		};
		if constexpr (prop::detail::is_equal_comparable_v<T>) {
			try {
				T t = call_source();
				if (prop::detail::is_equal(t, value)) {
					wn.p = nullptr;
				} else {
					value = std::move(t);
				}
			} catch (const prop::Property_expired &) {
				unbind();
			}
		} else {
			if constexpr (std::is_move_assignable_v<T>) {
				try {
					value = call_source();
				} catch (const prop::Property_expired &) {
					unbind();
				}
			} else {
				throw prop::Logic_error{"Trying to update a " + std::string{prop::type_name<prop::Property<T>>()} +
										" which is not move-assignable"};
			}
		}
#endif
	}

	Property<void> &Property<void>::operator=(std::convertible_to<std::move_only_function<void()>> auto &&f) {
		update_source(
			[source_ = std::forward<decltype(f)>(f)](std::span<prop::Property_link::Property_pointer>) { source_(); });
		return *this;
	}

	template <class T>
	Property(T &&t) -> Property<detail::inner_type_t<T>>;

	template <class U, class V>
		decltype(auto) operator +(U &&u, V &&v)
		requires((	prop::is_template_specialization_v<std::remove_cvref_t<U>, prop::Property>
					and
					(	requires { u.get() + std::forward<V>(v); }
						or
						requires { u.apply() + std::forward<V>(v); }
				    )
				 )
				 or
				 (	prop::is_template_specialization_v<std::remove_cvref_t<V>, prop::Property>
					and
					(	requires { std::forward<U>(u) + v.get(); }
						or
						requires { std::forward<U>(u) + v.apply(); }
					)
				 )
				)
	{
		if constexpr (prop::is_template_specialization_v<std::remove_cvref_t<U>, prop::Property>) {
			if constexpr (requires { u.get() + std::forward<V>(v); }) {
				return u.get() + std::forward<V>(v);
			} else {
				return u.apply() + std::forward<V>(v);
			}
		} else {
			if constexpr (requires { std::forward<U>(u) + v.get(); }) {
				return std::forward<U>(u) + v.get();
			} else {
				return std::forward<U>(u) + v.apply();
			}
		}
	}

#define PROP_OP(OP)                                                                                                    \
	template <class U, class V>                                                                 \
	decltype(auto) operator OP(U &&u, V &&v)                                                     \
		requires((	prop::is_template_specialization_v<std::remove_cvref_t<U>, prop::Property>    \
				  and                                                                              \
				  (	requires { u.get() OP std::forward<V>(v); }                                     \
				   or                                                                                \
				   requires { u.apply() OP std::forward<V>(v); }                                      \
				   )                                                                                   \
				  )                                                                                     \
				 or                                                                                      \
				 (	prop::is_template_specialization_v<std::remove_cvref_t<V>, prop::Property>            \
				  and                                                                                      \
				  (	requires { std::forward<U>(u) OP v.get(); }                                             \
				   or                                                                                        \
				   requires { std::forward<U>(u) OP v.apply(); }                                              \
				   )                                                                                           \
				  )                                                                                             \
				 )                                                                                               \
	{                      \
		if constexpr (prop::is_template_specialization_v<std::remove_cvref_t<U>, prop::Property>) {                    \
			if constexpr (requires { u.get() OP std::forward<V>(v); }) {                                               \
				return u.get() OP std::forward<V>(v);                                                                  \
			} else {                                                                                                   \
				return u.apply() OP std::forward<V>(v);                                                                \
			}                                                                                                          \
		} else {                                                                                                       \
			if constexpr (requires { std::forward<U>(u) OP v.get(); }) {                                               \
				return std::forward<U>(u) OP v.get();                                                                  \
			} else {                                                                                                   \
				return std::forward<U>(u) OP v.apply();                                                                \
			}                                                                                                          \
		}                                                                                                              \
	}

	//Binary ops: + - * / % ^ | = < > += -= *= /= %= ^= &= |= << >> >>= <<= == != <= >= <=> && || , ->* ( ) [ ]
	//PROP_OP(+)
	PROP_OP(-)
	PROP_OP(*)
	PROP_OP(/)
	PROP_OP(%)
	PROP_OP(^)
	PROP_OP(|)
	//PROP_OP(=) //done manually for more overloads
	PROP_OP(+=)
	PROP_OP(-=)
	PROP_OP(*=)
	PROP_OP(/=)
	PROP_OP(%=)
	PROP_OP(^=)
	PROP_OP(&=)
	PROP_OP(|=)
	PROP_OP(<<)
	PROP_OP(>>)
	PROP_OP(>>=)
	PROP_OP(<<=)
	//PROP_OP(<)
	//PROP_OP(>)
	//PROP_OP(==)
	//PROP_OP(!=)
	//PROP_OP(<=)
	//PROP_OP(>=)
	//PROP_OP(<=>)
	PROP_OP(&&)
	PROP_OP(||)
	//PROP_OP(,) //does not compile due to preprocessor limitations
	PROP_OP(->*)
//PROP_OP(()) //variadic parameters
//PROP_OP([]) //different syntax
#undef PROP_OP

#define PROP_BINOPS PROP_X(<=>) PROP_X(==) PROP_X(!=) PROP_X(<) PROP_X(<=) PROP_X(>) PROP_X(>=)
#define PROP_X(OP)                                                                                                     \
	template <class T, class U>                                                                                        \
		requires(prop::is_template_specialization_v<T, prop::Property> and                                             \
				 not prop::is_template_specialization_v<U, prop::Property>)                                            \
	auto operator OP(const T &lhs, const U &rhs)->decltype(lhs.get() OP rhs) {                                         \
		return lhs.get() OP rhs;                                                                                       \
	}                                                                                                                  \
	template <class T, class U>                                                                                        \
		requires(not prop::is_template_specialization_v<T, prop::Property> and                                         \
				 prop::is_template_specialization_v<U, prop::Property>)                                                \
	auto operator OP(const T &lhs, const U &rhs)->decltype(lhs OP rhs.get()) {                                         \
		return lhs OP rhs.get();                                                                                       \
	}                                                                                                                  \
	template <class T, class U>                                                                                        \
		requires(prop::is_template_specialization_v<T, prop::Property> and                                             \
				 prop::is_template_specialization_v<U, prop::Property>)                                                \
	auto operator OP(const T &lhs, const U &rhs)->decltype(lhs.get() OP rhs.get()) {                                   \
		return lhs.get() OP rhs.get();                                                                                 \
	}
	PROP_BINOPS
#undef PROP_X
#undef PROP_BINOPS

} // namespace prop

template <class U, class V>
decltype(auto) operator+(U &&u, V &&v)
	requires(prop::is_template_specialization_v<U, prop::Property> or
			 prop::is_template_specialization_v<V, prop::Property>)
{
	if constexpr (prop::is_template_specialization_v<U, prop::Property>) {
		if constexpr (requires { u.get() + std::forward<V>(v); }) {
			return u.get() + std::forward<V>(v);
		} else {
			return u.apply() + std::forward<V>(v);
		}
	} else {
		return std::forward<U>(u) + v.get();
	}
}
