/**
 * Created by veselink1.
 * Released under the MIT license.
 */

#ifndef REFL_HT_CLANG_UTILS_HPP
#define REFL_HT_CLANG_UTILS_HPP
#include <atomic>
#include <clang-c/Index.h>
#include "rht-fs.hpp"

std::ostream& operator<<(std::ostream& stream, const CXString& str)
{
    stream << clang_getCString(str);
    clang_disposeString(str);
    return stream;
}

namespace rht::util
{

std::string to_string(const CXString& str)
{
    std::string result{clang_getCString(str)};
    clang_disposeString(str);
    return result;
}

std::string to_string(const std::exception& e) 
{
    return std::string("(") + typeid(e).name() + "): " + e.what();
}

namespace detail
{
static std::atomic_bool log_info_enabled{false};
static struct : std::ostream
{

} dummy_ostream;
}

void enable_log_info()
{
    detail::log_info_enabled.store(true);
}

void disable_log_info()
{
    detail::log_info_enabled.store(false);
}

std::ostream& log_info()
{
    if (detail::log_info_enabled.load())
    {
        return std::cout << "I: ";
    }
    
    return detail::dummy_ostream;
}

std::ostream& log_error()
{
    return std::cerr << "ERR: ";
}

struct SourceLocation
{
    std::string filename;
    unsigned line;
    unsigned column;
};

std::ostream& operator<<(std::ostream& stream, const SourceLocation& loc)
{
    stream << loc.filename << ":" << loc.line << ":" << loc.column;
    return stream;
}

SourceLocation get_source_location(CXCursor c)
{
    CXSourceLocation location = clang_getCursorLocation(c);

    CXString filename;
    unsigned int line, column;

    clang_getPresumedLocation(location,& filename,& line,& column);

    return {to_string(filename), line, column};
}

template <typename T, size_t N>
constexpr size_t array_size(const T (&arr)[N])
{
    return N;
}

template <typename T>
bool contains(const T& item, std::initializer_list<T> items)
{
    for (auto&& other : items)
    {
        if (item == other)
            return true;
    }
    return false;
}

template <typename F>
bool visit_children(CXCursor cursor, F&& f)
{
    return clang_visitChildren(cursor, [](CXCursor child, CXCursor parent, CXClientData client_data) -> CXChildVisitResult {
        return (*reinterpret_cast<std::remove_reference_t<F> *>(client_data))(child, parent);
    }, &f);
}

std::string get_full_name(CXCursor cursor)
{
    std::vector<const char *> parents;
    CXCursor currentParent = clang_getCursorSemanticParent(cursor);
    while (util::contains(clang_getCursorKind(currentParent), {CXCursor_ClassDecl, CXCursor_StructDecl, CXCursor_Namespace}))
    {
        parents.push_back(clang_getCString(clang_getCursorDisplayName(currentParent)));
        currentParent = clang_getCursorSemanticParent(currentParent);
    }

    std::stringstream ss;
    for (auto it = parents.rbegin(); it != parents.rend(); ++it)
    {
        ss << *it << "::";
    }
    ss << clang_getCursorDisplayName(cursor);
    return ss.str();
}

std::string read_to_string(const char *filename)
{
    std::ifstream fs(filename);
    std::stringstream ss;

    std::string line;
    while (std::getline(fs, line))
    {
        ss << line << "\n";
    }
    return ss.str();
}

} // namespace rht::util

#endif // REFL_HT_CLANG_UTILS_HPP