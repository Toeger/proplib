#pragma once

#include "color.h"
#include "proplib/utility/utility.h"
#include "required_pointer.h"

#include <cassert>
#include <iostream>
#include <map>
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
		std::ostream &output = std::clog;
		std::string indent_with = "\t";
		int depth = 0;
	};

	class Property_link {
		public:
		using Property_pointer = prop::detail::Required_pointer<Property_link>;

		virtual std::string_view type() const = 0;
		virtual std::string value_string() const = 0;
		virtual bool has_source() const = 0;
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

		protected:
		void operator=(const Property_link &) = delete;
		void operator=(Property_link &&other);

		Property_link(Property_link &&other);

		virtual void update() = 0;
		virtual void unbind() {
			assert_status();
			for (std::size_t i = 0; i < explicit_dependencies + implicit_dependencies; i++) {
				dependencies[i]->remove_dependent(*this);
			}
			dependencies.erase(std::begin(dependencies),
							   std::begin(dependencies) + explicit_dependencies + implicit_dependencies);
			explicit_dependencies = implicit_dependencies = 0;
		}
		virtual std::string displayed_value() const {
			assert_status();
			return "<base>";
		}
		void read_notify() const;
		void write_notify();
		void update_start(Property_link *&previous_binding);
		void update_complete(Property_link *&previous_binding);

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
		void set_explicit_dependencies(std::vector<Property_pointer> deps);
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
			return std::span{dependencies}.subspan(0, static_cast<std::size_t>(explicit_dependencies));
		}
		std::span<const Property_pointer> get_implicit_dependencies() const {
			assert_status();
			return std::span{dependencies}.subspan(static_cast<std::size_t>(explicit_dependencies),
												   static_cast<std::size_t>(implicit_dependencies));
		}
		std::span<const Property_pointer> get_dependencies() const {
			assert_status();
			return std::span{dependencies}.subspan(
				0, static_cast<std::size_t>(explicit_dependencies + implicit_dependencies));
		}
		std::span<const Property_pointer> get_dependents() const {
			assert_status();
			return std::span{dependencies}.subspan(
				static_cast<std::size_t>(explicit_dependencies + implicit_dependencies));
		}

		//Stable_list get_stable_dependents() const;

#ifdef PROPERTY_NAMES
		std::string custom_name;
		std::string get_name() const {
			assert_status();
			std::string auto_name =
				prop::to_string(prop::Color::type) + std::string{type()} + prop::to_string(prop::Color::static_text) +
				"@" + prop::to_string(prop::Color::address) + prop::to_string(static_cast<const void *>(this)) +
				prop::to_string(prop::Color::reset);
			if (custom_name.empty()) {
				return auto_name;
			}
			return auto_name + ' ' + custom_name;
		}
#endif
		~Property_link();

		private:
#ifdef PROP_LIFETIMES
		enum class Property_state { pre, alive, post };
		static std::map<const Property_link *, Property_state> &lifetimes();
#endif
		void assert_status([[maybe_unused]] Property_state state = Property_state::alive) const {
#ifdef PROP_LIFETIMES
			assert(lifetimes()[this] == state);
#endif
		}
		void set_status([[maybe_unused]] Property_state state = Property_state::alive) const {
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

		mutable std::vector<Property_pointer> dependencies;
		mutable std::uint16_t explicit_dependencies = 0;
		mutable std::uint16_t implicit_dependencies = 0;
		static inline Property_link *current_binding;
		template <class T>
			requires(std::is_convertible_v<T *, prop::Property_link *>)
		friend class Tracking_list;
		friend prop::Dependency_tracer;
	};
	inline void Property_link::update() {
		assert_status();
		if (this != current_binding) {
			update();
		}
	}
	void swap(Property_link &lhs, Property_link &rhs);
} // namespace prop
