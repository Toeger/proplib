#pragma once

#include "color.h"
#include "property_decls.h"
#include "required_pointer.h"

#include <cassert>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#ifdef PROPERTY_NAMES
#include <string>
#endif

namespace prop {
	class Property_link;
	template <class T>
		requires(std::is_convertible_v<T *, prop::Property_link *>)
	class Tracking_list;
	class Dependency_tracer;

	struct Extended_status_data {
		std::ostream &output = std::cout;
		std::string indent_with = "\t";
		int depth = 0;
	};

	struct Update_data {
		std::size_t index;
	};

	struct Implicit_dependency_list {
		void read_notify(const prop::Property_link *p);
		void read_notify(prop::Required_pointer<prop::Property_link> p);
		const Update_data update_start(prop::Property_link *p);
		void update_end(const Update_data &update_data);
		prop::Property_link *current_binding() const;
		void remove(prop::Property_link *p);
		int binding_depth() const;

		private:
		std::vector<prop::Required_pointer<Property_link>> data;
		std::size_t current_index;
	};

	class Property_link {
		public:
		using Property_pointer = prop::Required_pointer<Property_link>;
		static inline std::ostream *debug_output;

		virtual std::string_view type() const;
		virtual std::string value_string() const;
		virtual bool has_source() const;
		void print_status(const Extended_status_data &esd = {}) const;
		std::string get_status() const;

		bool is_dependency_of(const Property_link &other) const {
			assert_status();

			return other.has_dependency(*this);
		}
		bool is_implicit_dependency_of(const Property_link &other) const {
			assert_status();
			return other.has_implicit_dependency(*this);
		}
		bool is_explicit_dependency_of(const Property_link &other) const {
			assert_status();
			return other.has_explicit_dependency(*this);
		}
		bool is_implicit_dependent_of(const Property_link &other) const {
			assert_status();
			return has_implicit_dependency(other);
		}
		bool is_explicit_dependent_of(const Property_link &other) const {
			assert_status();
			return has_explicit_dependency(other);
		}
		bool is_dependent_on(const Property_link &other) const {
			assert_status();
			return has_dependency(other);
		}
		bool has_dependency(const Property_link &other) const {
			assert_status();
			const auto end = std::begin(dependencies) + explicit_dependencies + implicit_dependencies;
			return std::find(std::begin(dependencies), end, &other) != end;
		}
		bool has_implicit_dependency(const Property_link &other) const {
			assert_status();
			const auto end = std::begin(dependencies) + explicit_dependencies + implicit_dependencies;
			return std::find(std::begin(dependencies) + explicit_dependencies, end, &other) != end;
		}
		bool has_explicit_dependency(const Property_link &other) const {
			assert_status();
			const auto end = std::begin(dependencies) + explicit_dependencies;
			return std::find(std::begin(dependencies), end, &other) != end;
		}
		bool has_dependent(const Property_link &other) const {
			assert_status();
			return std::find(std::begin(dependencies) + explicit_dependencies + implicit_dependencies,
							 std::end(dependencies), &other) != std::end(dependencies);
		}

		static const Property_link *get_current_binding() {
			return binding_data.current_binding();
		}

		std::string to_string() const;

		protected:
		void operator=(const Property_link &) = delete;
		void operator=(Property_link &&other);

		Property_link(Property_link &&other);

		virtual void update();
		virtual void unbind();
		virtual std::string displayed_value() const {
			assert_status();
			return "";
		}
		void read_notify() const;
		void write_notify();
		const prop::Update_data update_start();
		void update_complete(const Update_data &update_data);

		Property_link(const Property_link &) = delete;
		Property_link();
		Property_link(std::string_view type);
		Property_link(std::vector<Property_pointer> explicit_dependencies);

		friend void swap(Property_link &lhs, Property_link &rhs);

		void add_explicit_dependency(Property_pointer property) {
			assert_status();
			dependencies.insert(std::begin(dependencies) + explicit_dependencies++, property);
			property->add_dependent(*this);
		}
		void add_implicit_dependency(Property_pointer property) {
			assert_status();
			if (not has_dependency(*property)) {
				dependencies.insert(std::begin(dependencies) + explicit_dependencies + implicit_dependencies++,
									property);
				property->add_dependent(*this);
			}
		}
		void set_explicit_dependencies(std::vector<Property_pointer> &&deps);
		void replace_dependency(const Property_link &old_value, const Property_link &new_value) {
			assert_status();
			for (auto it = std::begin(dependencies),
					  end = std::begin(dependencies) + explicit_dependencies + implicit_dependencies;
				 it != end; ++it) {
				if (*it == &old_value) {
					old_value.remove_dependent(*this);
					*it = &new_value;
					new_value.add_dependent(*this);
				}
			}
		}
		std::span<const Property_pointer> get_explicit_dependencies() const {
			assert_status();
			return std::span{dependencies}.subspan(0, explicit_dependencies);
		}
		std::span<const Property_pointer> get_implicit_dependencies() const {
			assert_status();
			return std::span{dependencies}.subspan(explicit_dependencies, implicit_dependencies);
		}
		std::span<const Property_pointer> get_dependencies() const {
			assert_status();
			return std::span{dependencies}.subspan(0, explicit_dependencies + implicit_dependencies);
		}
		std::span<const Property_pointer> get_dependents() const {
			assert_status();
			return std::span{dependencies}.subspan(explicit_dependencies + implicit_dependencies);
		}

		//Stable_list get_stable_dependents() const;

#ifdef PROPERTY_NAMES
		std::string custom_name;
#endif
		~Property_link();

		private:
#ifdef PROP_LIFETIMES
		enum class Property_link_lifetime_status { pre, alive, post };
		friend std::ostream &operator<<(std::ostream &os, Property_link_lifetime_status state) {
			switch (state) {
				case Property_link_lifetime_status::pre:
					return os << "before construction";
				case Property_link_lifetime_status::alive:
					return os << "after construction and before destruction";
				case Property_link_lifetime_status::post:
					return os << "after destruction";
			}
			return os << "unknown";
		}
		static std::map<const Property_link *, Property_link_lifetime_status> &lifetimes();
#endif
		void assert_status(
			[[maybe_unused]] Property_link_lifetime_status state = Property_link_lifetime_status::alive) const {
#ifdef PROP_LIFETIMES
			if (lifetimes()[this] == state) {
				return;
			}
			std::stringstream ss;
			auto error =
				(std::stringstream{} << prop::Color::red << "Error" << prop::Color::static_text << ": Expected "
									 << this->to_string() << prop::Color::static_text << " to be in status "
									 << prop::Color::variable_value << state << prop::Color::static_text
									 << ", but it is in " << prop::Color::variable_value << lifetimes()[this]
									 << prop::Color::static_text << "." << prop::Color::reset)
					.str();
			std::clog << error << '\n';
			assert(lifetimes()[this] == state);
			throw std::runtime_error{std::move(error)};
#endif
		}
		void
		set_status([[maybe_unused]] Property_link_lifetime_status state = Property_link_lifetime_status::alive) const {
#ifdef PROP_LIFETIMES
			lifetimes()[this] = state;
#endif
		}
		void print_extended_status(const Extended_status_data &esd, int current_depth) const;
		void add_dependent(const Property_link &other) const {
			assert_status();
			if (not has_dependent(other)) {
				dependencies.push_back({&other, false});
			}
		}
		void remove_dependent(const Property_link &other) const {
			assert_status();
			for (auto it = std::begin(dependencies) + explicit_dependencies + implicit_dependencies;
				 it != std::end(dependencies); ++it) {
				if (*it == &other) {
					dependencies.erase(it);
					return;
				}
			}
		}
		void remove_implicit_dependency(const Property_link &other) {
			for (unsigned int index = 0; index < implicit_dependencies; index++) {
				if (dependencies[explicit_dependencies + index] == &other) {
					dependencies.erase(std::begin(dependencies) + explicit_dependencies + index);
					implicit_dependencies--;
					return;
				}
			}
		}
		void replace_dependent(const Property_link &old_value, const Property_link &new_value) {
			assert_status();
			for (std::size_t dependent_index = explicit_dependencies + implicit_dependencies;
				 dependent_index < std::size(dependencies); ++dependent_index) {
				if (dependencies[dependent_index] == &old_value) {
					dependencies[dependent_index] = &new_value;
					break;
				}
			}
		}

		std::string to_string(std::string_view type_name) const;

		public:
		mutable std::vector<Property_pointer> dependencies;

		mutable std::uint16_t explicit_dependencies = 0;
		mutable std::uint16_t implicit_dependencies = 0;

		private:
		static inline Implicit_dependency_list binding_data;
		template <class T>
			requires(std::is_convertible_v<T *, prop::Property_link *>)
		friend class Tracking_list;
		friend prop::Dependency_tracer;

		template <class T, class Function, class... Properties, std::size_t... indexes>
			requires(not std::is_same_v<T, void>)
		friend std::move_only_function<prop::Updater_result(
			prop::Property<T> &, std::span<const Property_link::Property_pointer> explicit_dependencies)>
		prop::detail::create_explicit_caller(Function &&function, std::index_sequence<indexes...>);

		friend prop::Implicit_dependency_list;
	};
	inline void Property_link::update() {
		assert_status();
		if (this != binding_data.current_binding()) {
			update();
		}
	}
	void swap(Property_link &lhs, Property_link &rhs);
} // namespace prop
