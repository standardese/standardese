// Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef STANDARDESE_CPP_TEMPLATE_HPP_INCLUDED
#define STANDARDESE_CPP_TEMPLATE_HPP_INCLUDED

#include <standardese/cpp_class.hpp>
#include <standardese/cpp_entity.hpp>
#include <standardese/cpp_type.hpp>

namespace standardese
{
    class cpp_template_ref
    {
    public:
        cpp_template_ref()
        : cpp_template_ref(cpp_cursor({}), "") {}

        /// Constructs it by giving the cursor to the template
        /// and the name as specified in the source.
        cpp_template_ref(cpp_cursor tmplate, cpp_name given)
        : given_(std::move(given)), declaration_(tmplate) {}

        /// Returns the name as specified in the source.
        const cpp_name& get_name() const STANDARDESE_NOEXCEPT
        {
            return given_;
        }

        /// Returns the full name with all scopes.
        cpp_name get_full_name() const;

        /// Returns a cursor to the declaration of the template.
        cpp_cursor get_declaration() const STANDARDESE_NOEXCEPT
        {
            return declaration_;
        }

    private:
        cpp_name given_;
        cpp_cursor declaration_;
    };

    class cpp_template_parameter
    : public cpp_parameter_base
    {
    public:
        static cpp_ptr<standardese::cpp_template_parameter> try_parse(translation_unit &tu, cpp_cursor cur);

        bool is_variadic() const STANDARDESE_NOEXCEPT
        {
            return variadic_;
        }

    protected:
        cpp_template_parameter(cpp_entity::type t, cpp_name name, cpp_raw_comment comment,
                               bool is_variadic)
        : cpp_parameter_base(t, std::move(name), std::move(comment)),
          variadic_(is_variadic) {}

    private:
        bool variadic_;
    };

    class cpp_template_type_parameter
    : public cpp_template_parameter
    {
    public:
        static cpp_ptr<cpp_template_type_parameter> parse(translation_unit &tu,  cpp_cursor cur);

        cpp_template_type_parameter(cpp_name name, cpp_raw_comment comment,
                                    cpp_type_ref def, bool is_variadic)
        : cpp_template_parameter(template_type_parameter_t,
                                 std::move(name), std::move(comment), is_variadic),
          default_(std::move(def)) {}

        bool has_default_type() const STANDARDESE_NOEXCEPT
        {
            return !default_.get_name().empty();
        }

        const cpp_type_ref& get_default_type() const STANDARDESE_NOEXCEPT
        {
            return default_;
        }

    private:
        cpp_type_ref default_;
    };

    class cpp_non_type_template_parameter
    : public cpp_template_parameter
    {
    public:
        static cpp_ptr<cpp_non_type_template_parameter> parse(translation_unit &tu,  cpp_cursor cur);

        cpp_non_type_template_parameter(cpp_name name, cpp_raw_comment comment,
                                        cpp_type_ref type, std::string default_value,
                                        bool is_variadic)
        : cpp_template_parameter(non_type_template_parameter_t,
                                 std::move(name), std::move(comment), is_variadic),
          type_(std::move(type)), default_(std::move(default_value)) {}

        const cpp_type_ref& get_type() const STANDARDESE_NOEXCEPT
        {
            return type_;
        }

        bool has_default_value() const STANDARDESE_NOEXCEPT
        {
            return !default_.empty();
        }

        const std::string& get_default_value() const STANDARDESE_NOEXCEPT
        {
            return default_;
        }

    private:
        cpp_type_ref type_;
        std::string default_;
    };

    class cpp_template_template_parameter
    : public cpp_template_parameter, public cpp_entity_container<cpp_template_parameter>
    {
    public:
        static cpp_ptr<cpp_template_template_parameter> parse(translation_unit &tu,  cpp_cursor cur);

        cpp_template_template_parameter(cpp_name name, cpp_raw_comment comment,
                                        cpp_template_ref def, bool is_variadic)
        : cpp_template_parameter(template_template_parameter_t,
                                 std::move(name), std::move(comment), is_variadic),
          default_(std::move(def)) {}

        void add_paramter(cpp_ptr<cpp_template_parameter> param)
        {
            cpp_entity_container::add_entity(std::move(param));
        }

        bool has_default_template() const STANDARDESE_NOEXCEPT
        {
            return !default_.get_name().empty();
        }

        const cpp_template_ref& get_default_template() const STANDARDESE_NOEXCEPT
        {
            return default_;
        }

    private:
        cpp_template_ref default_;
    };

    /// Returns whether cur refers to a full specialization of a template.
    /// cur must refer to a class or function.
    bool is_full_specialization(cpp_cursor cur);

    class cpp_function_base;

    class cpp_function_template
    : public cpp_entity, private cpp_entity_container<cpp_template_parameter>
    {
    public:
        static cpp_ptr<cpp_function_template> parse(translation_unit &tu,  cpp_name scope, cpp_cursor cur);

        cpp_function_template(cpp_name template_name, cpp_ptr<cpp_function_base> ptr);

        void add_template_parameter(cpp_ptr<cpp_template_parameter> param)
        {
            cpp_entity_container::add_entity(std::move(param));
        }

        const cpp_entity_container<cpp_template_parameter>& get_template_parameters() const STANDARDESE_NOEXCEPT
        {
            return *this;
        }

        const cpp_function_base& get_function() const STANDARDESE_NOEXCEPT
        {
            return *func_;
        }

    private:
        cpp_ptr<cpp_function_base> func_;
    };

    class cpp_function_template_specialization
    : public cpp_entity
    {
    public:
        static cpp_ptr<cpp_function_template_specialization> parse(translation_unit &tu,  cpp_name scope, cpp_cursor cur);

        cpp_function_template_specialization(cpp_name template_name, cpp_ptr<cpp_function_base> ptr);

        const cpp_function_base& get_function() const STANDARDESE_NOEXCEPT
        {
            return *func_;
        }

        const cpp_template_ref &get_primary_template() const STANDARDESE_NOEXCEPT
        {
            return template_;
        }

    private:
        cpp_ptr<cpp_function_base> func_;
        cpp_template_ref template_;
    };

    class cpp_class_template
    : public cpp_entity, private cpp_entity_container<cpp_template_parameter>
    {
    public:
        class parser : public cpp_entity_parser
        {
        public:
            parser(translation_unit &tu, cpp_name scope, cpp_cursor cur);

            void add_entity(cpp_entity_ptr ptr) override
            {
                parser_.add_entity(std::move(ptr));
            }

            cpp_name scope_name() override
            {
                return parser_.scope_name();
            }

            cpp_entity_ptr finish(const standardese::parser &par) override;

        private:
            cpp_class::parser parser_;
            cpp_ptr<cpp_class_template> class_;
        };

        cpp_class_template(cpp_name template_name, cpp_ptr<cpp_class> ptr);

        void add_template_parameter(cpp_ptr<cpp_template_parameter> param)
        {
            cpp_entity_container::add_entity(std::move(param));
        }

        const cpp_entity_container<cpp_template_parameter>& get_template_parameters() const STANDARDESE_NOEXCEPT
        {
            return *this;
        }

        const cpp_class &get_class() const STANDARDESE_NOEXCEPT
        {
            return *class_;
        }

    private:
        cpp_class_template(cpp_name scope, cpp_raw_comment comment);

        cpp_ptr<cpp_class> class_;
    };

    class cpp_class_template_full_specialization
    : public cpp_entity
    {
    public:
        class parser : public cpp_entity_parser
        {
        public:
            parser(translation_unit &p, cpp_name scope, cpp_cursor cur);

            void add_entity(cpp_entity_ptr ptr) override
            {
                parser_.add_entity(std::move(ptr));
            }

            cpp_name scope_name() override
            {
                return parser_.scope_name();
            }

            cpp_entity_ptr finish(const standardese::parser &par) override;

        private:
            cpp_class::parser parser_;
            cpp_ptr<cpp_class_template_full_specialization> class_;
        };

        cpp_class_template_full_specialization(cpp_name template_name, cpp_ptr<cpp_class> ptr,
                                               cpp_template_ref primary_template);

        const cpp_class &get_class() const STANDARDESE_NOEXCEPT
        {
            return *class_;
        }

        const cpp_template_ref &get_primary_template() const STANDARDESE_NOEXCEPT
        {
            return template_;
        }

    private:
        cpp_class_template_full_specialization(cpp_name scope, cpp_raw_comment comment);

        cpp_ptr<cpp_class> class_;
        cpp_template_ref template_;
    };

    class cpp_class_template_partial_specialization
    : public cpp_entity, public cpp_entity_container<cpp_template_parameter>
    {
    public:
    public:
        class parser : public cpp_entity_parser
        {
        public:
            parser(translation_unit &tu, cpp_name scope, cpp_cursor cur);

            void add_entity(cpp_entity_ptr ptr) override
            {
                parser_.add_entity(std::move(ptr));
            }

            cpp_name scope_name() override
            {
                return parser_.scope_name();
            }

            cpp_entity_ptr finish(const standardese::parser &par) override;

        private:
            cpp_class::parser parser_;
            cpp_ptr<cpp_class_template_partial_specialization> class_;
        };

        cpp_class_template_partial_specialization(cpp_name template_name, cpp_ptr<cpp_class> ptr,
                                                  cpp_template_ref primary_template);

        void add_template_parameter(cpp_ptr<cpp_template_parameter> param)
        {
            cpp_entity_container::add_entity(std::move(param));
        }

        const cpp_entity_container<cpp_template_parameter>& get_template_parameters() const STANDARDESE_NOEXCEPT
        {
            return *this;
        }

        const cpp_class &get_class() const STANDARDESE_NOEXCEPT
        {
            return *class_;
        }

        const cpp_template_ref &get_primary_template() const STANDARDESE_NOEXCEPT
        {
            return template_;
        }

    private:
        cpp_class_template_partial_specialization(cpp_name scope, cpp_raw_comment comment);

        cpp_ptr<cpp_class> class_;
        cpp_template_ref template_;

    };
} // namespace standardese

#endif // STANDARDESE_CPP_TEMPLATE_HPP_INCLUDED
