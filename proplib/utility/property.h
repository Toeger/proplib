#pragma once

#include "property_details.h"
#include "raii.h"

namespace prop {
	extern void (*on_property_severed)(prop::detail::Property_base *severed, prop::detail::Property_base *reason);
	extern void (*on_property_update_exception)(std::exception_ptr exception);

	template <class U>
	void print_status(const prop::Property<U> &p, std::ostream &os = std::clog);

	using detail::converts_to;

	template <class T>
	class Property final
		: public detail::Conversion_provider<T, false, decltype(converts_to(std::declval<T &>()))>,
		  public detail::Conversion_provider<T, true, decltype(converts_to(std::declval<const T &>()))>,
		  private prop::detail::Property_name_base<T> {
		using prop::detail::Property_base::explicit_dependencies;
		using prop::detail::Property_base::need_update;
		using prop::detail::Property_base::read_notify;
		using prop::detail::Property_base::sever_implicit_dependents;
		using prop::detail::Property_base::update_complete;
		using prop::detail::Property_base::update_start;
		using prop::detail::Property_base::write_notify;
		//using prop::detail::Property_base::need_update;

		T value{};
		std::move_only_function<prop::Value(T &, const prop::detail::Binding_list &)> source;
		struct Write_notifier;

		public:
		struct Value {
			T value;
		};

		struct Generator_data {
			Generator_data(prop::detail::Property_value_function<T> auto &&f)
				: value{f()}
				, source{prop::detail::make_direct_update_function(std::forward<decltype(f)>(f))} {}
			template <class... Properties>
			Generator_data(prop::detail::Property_value_function<T, Properties...> auto &&f,
						   Properties &&...properties) {
				detail::Property_function_binder<T> binder{std::forward<decltype(f)>(f),
														   std::forward<Properties>(properties)...};
				source = std::move(binder.function);
				explicit_dependencies = std::move(binder.explicit_dependencies);
			}
			T value;
			std::move_only_function<prop::Value(T &, const prop::detail::Binding_list &)> source;
			detail::Binding_list explicit_dependencies;
		};

		struct Generator {
			Generator_data generator;
		};

		struct Updater_data {
			template <class F>
			Updater_data(F &&f)
				: source{prop::detail::make_direct_update_function(std::forward<F>(f))} {}
			template <class F, class... Properties>
			Updater_data(F &&f, Properties &&...properties) {
				detail::Property_function_binder<T> binder{std::forward<F>(f), std::forward<Properties>(properties)...};
				source = std::move(binder.function);
				explicit_dependencies = std::move(binder.explicit_dependencies);
			}
			std::move_only_function<prop::Value(T &, const prop::detail::Binding_list &)> source;
			detail::Binding_list explicit_dependencies;
		};

		struct Updater {
			Updater_data updater;
		};

		public:
		Property();
		Property(const Property<T> &other);
		Property(Property<T> &&other);
		template <class... Args>
		Property(Args &&...args)
			requires std::constructible_from<T, Args &&...>;
		Property(Value &&v);
		Property(Generator &&generator);
		Property(prop::detail::Property_value_function<T> auto &&f);
		Property(prop::detail::Value_result_function<T> auto &&f, T t);
		Property(prop::detail::Property_function_binder<T> binder);
		~Property() {
			//prevents infinite recursion when source is a lambda that captures a property by value and depends on it
			auto{std::move(source)};
		}

		Property &operator=(const Property<T> &other);
		Property &operator=(Property<T> &&other);
		template <class U>
		Property &operator=(U &&u)
			requires std::is_assignable_v<T &, U &&>;
		Property &operator=(Value &&v);
		Property &operator=(Generator &&generator);
		Property &operator=(Updater &&updater);
		Property &operator=(prop::detail::Property_value_function<T> auto &&f);
		Property &operator=(prop::detail::Value_result_function<T> auto &&f);
		Property &operator=(prop::detail::Property_function_binder<T> binder);

		void set(T t);
		const T &get() const;
		bool is_bound() const;
		void unbind() final override;
		void sever();
		Write_notifier apply();
		template <class Functor>
		std::invoke_result_t<Functor, T &> apply(Functor &&f);
		template <class Functor>
		std::invoke_result_t<Functor, T &> apply_guarded(Functor &&f);

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
		void print_status(std::ostream &os = std::clog) const {
			prop::print_status(*this, os);
		}

		template <class U>
		operator U() const
			requires(std::convertible_to<const T &, U> and not std::same_as<T, std::remove_cvref_t<U>>)
		{
			read_notify();
			return static_cast<U>(value);
		}
		operator const T &() const;
		template <class Widget>
			requires(std::is_same_v<T, prop::Self>)
		operator Widget &() {
			return value.operator Widget &();
		}

		const T *operator->() const {
			this->read_notify();
			return &value;
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
		using prop::detail::Property_base::custom_name;
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
		void update_source(std::move_only_function<prop::Value(T &, const prop::detail::Binding_list &)> f);
		void update() override final;
		friend class Binding;
		friend prop::detail::Property_base *detail::get_property_base_pointer<T>(const Property &p);
		friend prop::detail::Property_base *detail::get_property_base_pointer<T>(const Property *p);
		template <class T_, class Function, class... Properties, std::size_t... indexes>
			requires(not std::is_same_v<T_, void>)
		friend std::move_only_function<prop::Value(T_ &, const prop::detail::Binding_list &)>
		prop::detail::create_explicit_caller(Function &&function, std::index_sequence<indexes...>);
		template <class T_, class Function, class... Properties, std::size_t... indexes>
			requires(std::is_same_v<T_, void>)
		friend std::move_only_function<void(const prop::detail::Binding_list &)>
		prop::detail::create_explicit_caller(Function &&function, std::index_sequence<indexes...>);
		template <class U>
		friend void prop::print_status(const prop::Property<U> &p, std::ostream &os);
		template <class U>
		friend class Property;
	};

	void print_status(const prop::Property<void> &p, std::ostream &os = std::clog);

	template <>
	class Property<void> final : prop::detail::Property_name_base<prop::Property<void>> {
		public:
		Property();
		Property(std::convertible_to<std::move_only_function<void()>> auto &&f);
		Property &operator=(std::convertible_to<std::move_only_function<void()>> auto &&f);
		Property &operator=(prop::detail::Property_function_binder<void> binder);
		template <class... Property_types,
				  prop::detail::Property_update_function<void, const Property<Property_types> *...> Function>
		void bind(Function &&source, Property<Property_types> &...properties);
		bool is_bound() const;
		void unbind() final override;

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
		using prop::detail::Property_base::custom_name;
#endif

		private:
		void update_source(std::move_only_function<void(const prop::detail::Binding_list &)> f);
		void update() override final;
		std::move_only_function<void(const prop::detail::Binding_list &)> source;
		friend class Binding;
		friend Property_base *detail::get_property_base_pointer<void>(const Property &p);
		friend Property_base *detail::get_property_base_pointer<void>(const Property *p);
		template <class T, class Function, class... Properties, std::size_t... indexes>
			requires(not std::is_same_v<T, void>)
		friend std::move_only_function<prop::Value(T &, const prop::detail::Binding_list &)>
		prop::detail::create_explicit_caller(Function &&function, std::index_sequence<indexes...>);
		template <class T, class Function, class... Properties, std::size_t... indexes>
			requires(std::is_same_v<T, void>)
		friend std::move_only_function<void(const prop::detail::Binding_list &)>
		prop::detail::create_explicit_caller(Function &&function, std::index_sequence<indexes...>);
		friend void prop::print_status(const prop::Property<void> &p, std::ostream &os);
		template <class U>
		friend class Property;
	};

	template <class U>
	bool Property<void>::is_implicit_dependency_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_implicit_dependency_of(other);
	}

	template <class U>
	bool Property<void>::is_explicit_dependency_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_explicit_dependency_of(other);
	}

	template <class U>
	bool Property<void>::is_dependency_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_dependency_of(other);
	}

	template <class U>
	bool Property<void>::is_implicit_dependent_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_implicit_dependent_of(other);
	}

	template <class U>
	bool Property<void>::is_explicit_dependent_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_explicit_dependent_of(other);
	}

	template <class U>
	bool Property<void>::is_dependent_on(const Property<U> &other) const {
		return prop::detail::Property_base::is_dependent_on(other);
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
		: value{} {}

	template <class T>
	Property<T>::Property(const Property<T> &other)
		: value{other.value} {
		other.read_notify();
	}
	template <class T>
	Property<T>::Property(Property<T> &&other)
		: value{std::move(other.value)}
		, source{std::move(other.source)} {
		static_cast<prop::detail::Property_base &>(*this) = static_cast<prop::detail::Property_base &&>(other);
	}

	template <class T>
	Property<T>::Property(prop::detail::Property_function_binder<T> binder) {
		*this = std::move(binder);
	}

	template <class T>
	Property<T> &Property<T>::operator=(Property<T> &&other) {
		value = std::move(other.value);
		source = std::move(other.source);
		static_cast<prop::detail::Property_base &>(*this) = static_cast<prop::detail::Property_base &&>(other);
		return *this;
	}

	template <class T>
	Property<T>::Property(prop::detail::Property_value_function<T> auto &&f)
		: value{[&] {
			if constexpr (std::is_default_constructible_v<T>) {
				return T{};
			} else {
				return f();
			}
		}()} {
		update_source(prop::detail::make_direct_update_function<T>(std::forward<decltype(f)>(f)));
	}

	template <class T>
	Property<T>::Property(prop::detail::Value_result_function<T> auto &&f, T t)
		: value{std::move(t)} {
		update_source(prop::detail::make_direct_update_function<T>(std::forward<decltype(f)>(f)));
	}

	template <class T>
	template <class... Args>
	Property<T>::Property(Args &&...args)
		requires std::constructible_from<T, Args &&...>
		: value{std::forward<Args>(args)...} {}

	template <class T>
	Property<T>::Property(Value &&v)
		: value{std::move(v.value)} {}

	template <class T>
	Property<T>::Property(Generator &&generator)
		: value{std::move(generator.generator.value)} {
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
	Property<T> &Property<T>::operator=(Generator &&generator) {
		return *this;
	}

	template <class T>
	Property<T> &Property<T>::operator=(prop::detail::Property_value_function<T> auto &&f) {
		return *this = prop::detail::Property_function_binder<T>{std::forward<decltype(f)>(f)};
	}
	template <class T>
	Property<T> &Property<T>::operator=(prop::detail::Value_result_function<T> auto &&f) {
		return *this = prop::detail::Property_function_binder<T>{std::forward<decltype(f)>(f)};
	}
	template <class T>
	Property<T> &Property<T>::operator=(prop::detail::Property_function_binder<T> binder) {
		prop::detail::Property_base::set_explicit_dependencies(std::move(binder.explicit_dependencies));
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
		prop::detail::Property_base::unbind();
	}

	template <class T>
	void Property<T>::sever() {
		sever_implicit_dependents();
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
		return prop::detail::Property_base::is_implicit_dependency_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_explicit_dependency_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_explicit_dependency_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_dependency_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_dependency_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_implicit_dependent_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_implicit_dependent_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_explicit_dependent_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_explicit_dependent_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_dependent_on(const Property<U> &other) const {
		return prop::detail::Property_base::is_dependent_on(other);
	}

	template <class T>
	template <class Functor>
	std::invoke_result_t<Functor, T &> Property<T>::apply_guarded(Functor &&f) {
		Write_notifier wn{this};
		prop::detail::RAII guard{[] { prop::detail::Property_base::current_binding = nullptr; },
								 [binding = prop::detail::Property_base::current_binding] {
									 prop::detail::Property_base::current_binding = binding;
								 }};
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
	void Property<T>::update_source(std::move_only_function<prop::Value(T &, const prop::detail::Binding_list &)> f) {
		std::swap(f, source);
		update();
	}

	template <class T>
	void Property<T>::update() {
		prop::detail::RAII updater{[this] {
									   update_start(); //
								   },
								   [this] {
									   update_complete();
									   need_update = false;
								   }};
		try {
			if (source(value, explicit_dependencies) == prop::Value::changed) {
				updater.early_exit();
				write_notify();
			}
		} catch (const prop::Property_expired &) {
			unbind();
		} catch (...) {
			unbind();
			throw;
		}
#if 0
		Write_notifier wn{this};
		auto call_source = [this] {
			prop::detail::RAII updater{[this] { update_start(); },
									   [this] {
										   update_complete();
										   need_update = false;
									   }};
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
				throw prop::Logic_error{"Trying to update a " + std::string{prop::type_name<T>()} +
										" which is not move-assignable"};
			}
		}
#endif
	}

	template <class T>
	void print_status(const prop::Property<T> &p, std::ostream &os) {
		os << prop::Color::static_text;
#ifdef PROPERTY_NAMES
		os << "Property   " << p.get_name() << '\n';
#else
		os << "Property   " << prop::Color::address << &p << '\n';
#endif
		os << prop::Color::static_text << "\tvalue: " << prop::Color::reset << prop::detail::Printer{p.value} << "\n";
		os << prop::Color::static_text << "\tsource: " << prop::Color::reset << (p.source ? "Yes" : "No") << "\n";
		os << prop::Color::static_text << "\tExplicit dependencies: [" << prop::Color::reset << p.explicit_dependencies
		   << prop::Color::static_text << "]\n";
		os << prop::Color::static_text << "\tImplicit dependencies: [" << prop::Color::reset
		   << p.get_implicit_dependencies() << prop::Color::static_text << "]\n";
		os << prop::Color::static_text << "\tDependents: [" << prop::Color::reset << p.get_dependents()
		   << prop::Color::static_text << "]\n"
		   << prop::Color::reset;
	}

	Property<void> &Property<void>::operator=(std::convertible_to<std::move_only_function<void()>> auto &&f) {
		update_source([source_ = std::forward<decltype(f)>(f)](const prop::detail::Binding_list &) { source_(); });
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
