// Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <fstream>
#include <iostream>

#include <boost/program_options.hpp>

#include "filesystem.hpp"
#include "generator.hpp"
#include "thread_pool.hpp"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

constexpr auto terminal_width = 100u; // assume 100 columns for terminal help text

inline bool default_msvc_comp() noexcept
{
#ifdef _MSC_VER
    return true;
#else
    return false;
#endif
}

void print_version(const char* exe_name)
{
    std::clog << exe_name << " version " << STANDARDESE_VERSION_MAJOR << '.'
              << STANDARDESE_VERSION_MINOR << '\n';
    std::clog << "Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>\n";
    std::clog << '\n';
}

void print_usage(const char* exe_name, const po::options_description& generic,
                 const po::options_description& configuration)
{
    std::clog << "Usage: " << exe_name << " [options] inputs\n";
    std::clog << '\n';
    std::clog << generic << '\n';
    std::clog << '\n';
    std::clog << configuration << '\n';
}

po::variables_map get_options(int argc, char* argv[], const po::options_description& generic,
                              const po::options_description& configuration)
{
    po::variables_map map;

    po::options_description input("");
    input.add_options()("input-files", po::value<std::vector<fs::path>>(), "input files");
    po::positional_options_description input_pos;
    input_pos.add("input-files", -1);

    po::options_description cmd;
    cmd.add(generic).add(configuration).add(input);

    auto cmd_result = po::command_line_parser(argc, argv)
                          .options(cmd)
                          .positional(input_pos)
                          .allow_unregistered()
                          .run();
    po::store(cmd_result, map);
    po::notify(map);

    auto               iter = map.find("config");
    po::parsed_options file_result(nullptr);
    if (iter != map.end())
    {
        auto          path = iter->second.as<fs::path>();
        std::ifstream config(path.string());
        if (!config.is_open())
            throw std::runtime_error("config file '" + path.generic_string() + "' not found");

        po::options_description conf;
        conf.add(configuration);

        file_result = po::parse_config_file(config, configuration, true);
        po::store(file_result, map);
        po::notify(map);
    }

    return map;
}

bool has_option(const po::variables_map& map, const char* name)
{
    return map.count(name) != 0u;
}

template <typename T>
type_safe::optional<T> get_option(const po::variables_map& map, const char* name)
{
    auto iter = map.find(name);
    if (iter != map.end())
        return iter->second.as<T>();
    else
        return type_safe::nullopt;
}

inline cppast::cpp_standard parse_standard(const std::string& str)
{
    using cppast::cpp_standard;

    if (str == "c++98")
        return cpp_standard::cpp_98;
    else if (str == "c++03")
        return cpp_standard::cpp_03;
    else if (str == "c++11")
        return cpp_standard::cpp_11;
    else if (str == "c++14")
        return cpp_standard::cpp_14;
    else if (str == "c++1z" || str == "c++17")
        return cpp_standard::cpp_1z;
    else
        throw std::invalid_argument("invalid C++ standard '" + str + "'");
}

cppast::libclang_compile_config get_compile_config(const po::variables_map& options)
{
    cppast::libclang_compile_config config;

    cppast::compile_flags flags;
    if (auto gnu_ext = get_option<bool>(options, "compilation.gnu_extensions"))
        flags.set(cppast::compile_flag::gnu_extensions, gnu_ext.value());
    if (auto ms_ext = get_option<bool>(options, "compilation.ms_extensions"))
        flags.set(cppast::compile_flag::ms_extensions, ms_ext.value());
    if (auto ms_comp = get_option<bool>(options, "compilation.ms_compatibility"))
        flags.set(cppast::compile_flag::ms_compatibility, ms_comp.value());

    auto standard = parse_standard(options.at("compilation.standard").as<std::string>());
    config.set_flags(standard, flags);

    if (auto includes = get_option<std::vector<std::string>>(options, "compilation.include_dir"))
        for (auto& include : includes.value())
            config.add_include_dir(include);

    if (auto macros = get_option<std::vector<std::string>>(options, "compilation.macro_definition"))
        for (auto& macro : macros.value())
        {
            auto pos = macro.find('=');
            if (pos == std::string::npos)
                config.define_macro(macro, "");
            else
                config.define_macro(macro.substr(0, pos), macro.substr(pos + 1));
        }

    if (auto macros
        = get_option<std::vector<std::string>>(options, "compilation.macro_undefinition"))
        for (auto& macro : macros.value())
            config.undefine_macro(macro);

    if (auto features = get_option<std::vector<std::string>>(options, "compilation.feature"))
        for (auto& feature : features.value())
            config.enable_feature(feature);

    return config;
}

type_safe::optional<cppast::libclang_compilation_database> get_compilation_database(
    const po::variables_map& options)
{
    if (auto dir = get_option<std::string>(options, "compilation.commands_dir"))
        return cppast::libclang_compilation_database(dir.value());
    else
        return type_safe::nullopt;
}

std::vector<standardese_tool::input_file> get_input(const po::variables_map& options)
{
    auto source_ext = get_option<std::vector<std::string>>(options, "input.source_ext").value();
    auto blacklist_ext
        = get_option<std::vector<std::string>>(options, "input.blacklist_ext").value();
    auto blacklist_files
        = get_option<std::vector<std::string>>(options, "input.blacklist_file").value();
    auto blacklist_dirs
        = get_option<std::vector<std::string>>(options, "input.blacklist_dir").value();
    auto blacklist_dotfiles = get_option<bool>(options, "input.blacklist_dotfiles").value();
    auto force_blacklist    = get_option<bool>(options, "input.force_blacklist").value();

    auto input_files = get_option<std::vector<fs::path>>(options, "input-files");
    if (!input_files)
        throw std::invalid_argument("no input files specified");

    std::vector<standardese_tool::input_file> files;
    for (auto& file : input_files.value())
        standardese_tool::handle_path(file, source_ext, blacklist_ext, blacklist_files,
                                      blacklist_dirs, blacklist_dotfiles, force_blacklist,
                                      [&](bool, const fs::path& path, const fs::path& relative) {
                                          files.push_back({path, relative});
                                      });

    return files;
}

standardese::comment::config get_comment_config(const po::variables_map& options)
{
    standardese::comment::config config(
        get_option<char>(options, "comment.command_character").value());

    // TODO: handle command overrides

    return config;
}

standardese::synopsis_config get_synopsis_config(const po::variables_map& options)
{
    standardese::synopsis_config config;

    if (auto tab = get_option<unsigned>(options, "output.tab_width"))
        config.set_tab_width(tab.value());

    config.set_flag(standardese::synopsis_config::show_complex_noexcept,
                    get_option<bool>(options, "output.show_complex_noexcept").value());
    config.set_flag(standardese::synopsis_config::show_macro_replacement,
                    get_option<bool>(options, "output.show_macro_replacement").value());
    config.set_flag(standardese::synopsis_config::show_group_output_section,
                    get_option<bool>(options, "output.show_group_output_section").value());

    return config;
}

standardese::generation_config get_generation_config(const po::variables_map& options)
{
    standardese::generation_config config;

    config.set_flag(standardese::generation_config::document_uncommented,
                    !get_option<bool>(options, "input.require_comment").value());
    config.set_flag(standardese::generation_config::inline_doc,
                    get_option<bool>(options, "output.inline_doc").value());

    auto order = get_option<std::string>(options, "output.entity_index_order").value();
    if (order == "namespace_inline_sorted")
        config.set_order(standardese::entity_index::namespace_inline_sorted);
    else if (order == "namespace_external")
        config.set_order(standardese::entity_index::namespace_external);
    else
        throw std::invalid_argument("unknown entity_index_order '" + order + "'");

    return config;
}

std::vector<std::pair<standardese::markup::generator, const char*>> get_formats(
    const po::variables_map& options)
{
    std::vector<std::pair<standardese::markup::generator, const char*>> formats;

    auto link_prefix    = get_option<std::string>(options, "output.link_prefix").value_or("");
    auto link_extension = get_option<std::string>(options, "output.link_extension");

    auto option = get_option<std::vector<std::string>>(options, "output.format").value();
    for (auto& format : option)
        if (format == "html")
            formats.emplace_back(standardese::markup::html_generator(link_prefix,
                                                                     link_extension.value_or(
                                                                         "html")),
                                 "html");
        else if (format == "xml")
            formats.emplace_back(standardese::markup::xml_generator(), "xml");
        else if (format == "commonmark")
            formats.emplace_back(standardese::markup::markdown_generator(false, link_prefix,
                                                                         link_extension.value_or(
                                                                             "md")),
                                 "md");
        else if (format == "commonmark_html")
            formats.emplace_back(standardese::markup::markdown_generator(true, link_prefix,
                                                                         link_extension.value_or(
                                                                             "md")),
                                 "md");
        else if (format == "text")
            formats.emplace_back(standardese::markup::text_generator(), "txt");
        else
            throw std::invalid_argument("unknown format '" + format + "'");

    return formats;
}

standardese::entity_blacklist get_blacklist(const po::variables_map& options)
{
    standardese::entity_blacklist blacklist(
        get_option<bool>(options, "input.extract_private").value());

    auto blacklist_ns
        = get_option<std::vector<std::string>>(options, "input.blacklist_namespace").value();
    for (auto& ns : blacklist_ns)
        blacklist.blacklist_namespace(ns);

    return blacklist;
}

void register_external_documentations(standardese::linker& l, const po::variables_map& options)
{
    l.register_external("std", "http://en.cppreference.com/mwiki/"
                               "index.php?title=Special%3ASearch&search=$$");

    auto external = get_option<std::vector<std::string>>(options, "comment.external_doc").value();
    for (auto& arg : external)
    {
        auto equal = arg.find('=');
        if (equal == std::string::npos)
            throw std::invalid_argument("invalid format for external doc '" + arg + "'");

        auto ns_name = arg.substr(0, equal);
        auto url     = arg.substr(equal + 1u);
        l.register_external(std::move(ns_name), std::move(url));
    }
}

int main(int argc, char* argv[])
{
    // clang-format off
    po::options_description generic("Generic options", terminal_width), configuration("Configuration", terminal_width);
    generic.add_options()
        ("version,V", "prints version information and exits")
        ("help,h", "prints this help message and exits")
        ("config,c", po::value<fs::path>(), "read options from additional config file as well")
        ("verbose,v", po::value<bool>()->implicit_value(true)->default_value(false),
         "prints more information")
        ("jobs,j", po::value<unsigned>()->default_value(standardese_tool::default_no_threads()),
         "sets the number of threads to use");

    configuration.add_options()
        ("input.source_ext",
         po::value<std::vector<std::string>>()
         ->default_value({".h", ".hpp", ".h++", ".hxx"}, "(common C++ header file extensions)"),
         "file extensions that are treated as header files and where files will be parsed")
        ("input.blacklist_ext",
         po::value<std::vector<std::string>>()->default_value({}, "(none)"),
         R"(file extension that is forbidden (e.g. ".md"; "." for no extension))")
        ("input.blacklist_file",
         po::value<std::vector<std::string>>()->default_value({}, "(none)"),
         "file that is forbidden, relative to traversed directory")
        ("input.blacklist_dir",
         po::value<std::vector<std::string>>()->default_value({}, "(none)"),
         "directory that is forbidden, relative to traversed directory")
        ("input.blacklist_dotfiles",
         po::value<bool>()->implicit_value(true)->default_value(true),
         "whether or not dotfiles are blacklisted")
        ("input.blacklist_namespace",
         po::value<std::vector<std::string>>()->default_value({}, "(none)"),
         "C++ namespace names (with all children) that are forbidden")
        ("input.force_blacklist",
         po::value<bool>()->implicit_value(true)->default_value(false),
         "force the blacklist for explicitly given files")
        ("input.require_comment",
         po::value<bool>()->implicit_value(true)->default_value(true),
         "only generates documentation for entities that have a documentation comment")
        ("input.extract_private",
         po::value<bool>()->implicit_value(true)->default_value(false),
         "whether or not to document private entities")

        ("compilation.commands_dir", po::value<std::string>(),
         "the directory where a compile_commands.json is located, its options have lower priority than the other ones")
        ("compilation.standard", po::value<std::string>()->default_value("c++14"),
         "the C++ standard to use for parsing, valid values are c++98/03/11/14/1z/17")
        ("compilation.include_dir,I", po::value<std::vector<std::string>>(),
         "adds an additional include directory to use for parsing")
        ("compilation.macro_definition,D", po::value<std::vector<std::string>>(),
         "adds an implicit #define before parsing")
        ("compilation.macro_undefinition,U", po::value<std::vector<std::string>>(),
         "adds an implicit #undef before parsing")
         ("compilation.feature,f", po::value<std::vector<std::string>>(),
         "enable a custom feature (-fXX flag)")
        ("compilation.gnu_extensions",
         po::value<bool>()->implicit_value(true)->default_value(false),
         "enable/disable GNU extension support (-std=gnu++XX vs -std=c++XX)")
        ("compilation.ms_extensions",
         po::value<bool>()->implicit_value(true)->default_value(default_msvc_comp()),
         "enable/disable MSVC extension support (-fms-extensions)")
        ("compilation.ms_compatibility",
         po::value<bool>()->implicit_value(true)->default_value(default_msvc_comp()),
         "enable/disable MSVC compatibility (-fms-compatibility)")

        ("comment.command_character", po::value<char>()->default_value(standardese::comment::config::default_command_character()),
         "character used to introduce special commands")
        ("comment.cmd_name_", po::value<std::string>(), // TODO
         "override name for the command following the name_ (e.g. comment.cmd_name_requires=require)")
        ("comment.external_doc", po::value<std::vector<std::string>>()->default_value({}, ""),
         "syntax is namespace=url, supports linking to a different URL for entities in a certain namespace")

        // TODO
        ("template.default_template", po::value<std::string>()->default_value("", ""),
         "set the default template for all output")
        ("template.delimiter_begin", po::value<std::string>()->default_value("{{"),
         "set the template delimiter begin string")
        ("template.delimiter_end", po::value<std::string>()->default_value("}}"),
         "set the template delimiter end string")
        ("template.cmd_name_", po::value<std::string>(),
         "override the name for the template command following the name_ (e.g. template.cmd_name_if=my_if);"
         "standardese prefix will be added automatically")

        ("output.prefix",
         po::value<std::string>()->default_value(""),
         "a prefix that will be added to all output files")
        ("output.format",
         po::value<std::vector<std::string>>()->default_value(std::vector<std::string>{"html"}, "{html}"),
         "the output format used (html, commonmark, commonmark_html, xml, text)")
        ("output.link_extension", po::value<std::string>(),
         "the file extension of the links to entities, useful if you convert standardese output to a different format and change the extension")
        ("output.link_prefix", po::value<std::string>(),
        "a prefix that will be added to all links, if not specified they'll be relative links")
        ("output.entity_index_order", po::value<std::string>()->default_value("namespace_inline_sorted"),
         "how the namespaces are handled in the entity index: namespace_inline_sorted (sorted inline with all others), "
         "namespace_external (namespaces in top-level list only, sorted by the end position in the source file)")
        ("output.section_name_", po::value<std::string>(), // TODO
         "override output name for the section following the name_ (e.g. output.section_name_requires=Require)")
        ("output.tab_width", po::value<unsigned>()->default_value(standardese::synopsis_config::default_tab_width()),
         "the tab width (i.e. number of spaces, won't emit tab) of the code in the synthesis")
        ("output.inline_doc", po::value<bool>()->default_value(true)->implicit_value(true),
         "whether or not some entity documentation (parameters etc.) will be shown inline")
        ("output.show_complex_noexcept", po::value<bool>()->default_value(true)->implicit_value(true),
         "whether or not complex noexcept expressions will be shown in the synopsis or replaced by \"see below\"")
        ("output.show_macro_replacement", po::value<bool>()->default_value(false)->implicit_value(true),
         "whether or not the replacement of macros will be shown")
        ("output.show_group_output_section", po::value<bool>()->default_value(true)->implicit_value(true),
         "whether or not member groups have an implicit output section");
    // clang-format on

    try
    {
        auto options = get_options(argc, argv, generic, configuration);

        if (has_option(options, "version"))
            print_version(argv[0]);
        else if (has_option(options, "help"))
            print_usage(argv[0], generic, configuration);
        else
        {
            auto no_threads = get_option<unsigned>(options, "jobs").value();

            auto compile_config = get_compile_config(options);
            auto database       = get_compilation_database(options);
            auto input          = get_input(options);

            auto comment_config    = get_comment_config(options);
            auto synopsis_config   = get_synopsis_config(options);
            auto generation_config = get_generation_config(options);

            auto blacklist = get_blacklist(options);

            auto formats = get_formats(options);
            auto prefix  = get_option<std::string>(options, "output.prefix").value();

            standardese::linker linker;
            register_external_documentations(linker, options);

            try
            {
                cppast::cpp_entity_index index;

                std::clog << "parsing C++ files...\n";
                auto parsed
                    = standardese_tool::parse(compile_config, database, input, index, no_threads);
                if (!parsed)
                    return 1;

                std::clog << "parsing documentation comments...\n";
                auto comments
                    = standardese_tool::parse_comments(comment_config, parsed.value(), no_threads);
                auto files
                    = standardese_tool::build_files(comments, index, std::move(parsed.value()),
                                                    blacklist, no_threads);

                std::clog << "generating documentation...\n";
                auto docs = standardese_tool::generate(generation_config, synopsis_config, comments,
                                                       index, linker, files, no_threads);

                for (auto& format : formats)
                {
                    std::clog << "writing files in format '" << format.second << "'...\n";

                    auto format_prefix
                        = formats.size() > 1u ? std::string(format.second) + '/' + prefix : prefix;
                    if (!format_prefix.empty())
                        fs::create_directories(fs::path(format_prefix).parent_path());
                    standardese_tool::write_files(docs, format.first, std::move(format_prefix),
                                                  format.second, no_threads);
                }
            }
            catch (std::exception& ex)
            {
                std::cerr << "error: " << ex.what() << '\n';
            }
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << "error: " << ex.what() << '\n';
        std::cerr << '\n';
        print_usage(argv[0], generic, configuration);
        return 1;
    }
}
