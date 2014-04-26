#ifndef COSMOSIS_DATABLOCK_HH
#define COSMOSIS_DATABLOCK_HH

//----------------------------------------------------------------------
// The class DataBlock provides the facilities to pass data from a
// Sampler to physics modules. It provides storage of and access to
// "named values". The value identification scheme is two-part: the
// full specificiation of the identifier consists of the specification
// of a 'section' and a 'name'. Values can be any of the following types:
//
//    1) int
//    2) double
//    3) std::string
//    4) std::complex<double>
//    5) std::vector<int>
//    6) std::vector<double>
//    7) std::vector<std::string>
//    8) std::vector<std::complex<double>>
//
// The class is designed to support both idiomatic C++ usage
// (recommended for C++ client code) and to support a C interface. For
// this reasons, much of the functionality of the interface is
// presented twice, once with an interface that is more convenient to
// use from C++, which is permitted to throw exceptions, and again
// with an interface that is guaranteed never to throw exceptions.
//
// The C interface does not provide direct access to the memory
// controlled by the DataBlock; it provides the user with a copy of
// the data, copied to a memory location of the user's
// specification. For any array obtained from the DataBlock, the C
// user must deallocate the array when he is done with it. The
// deallocation does not need to be done for arrays that are on the
// stack, into which DataBlock copies the data.
//
//  TODO: Add support for 2-dimensional arrays of the numeric types.
//
//----------------------------------------------------------------------

#include <string>
#include <map>
#include <cctype>
#include <ostream>

#include "datablock_status.h"
#include "section.hh"
#include "datablock_logging.h"

#define OPTION_SECTION "module_options"

namespace cosmosis
{
  inline
  void downcase(std::string& s) { for (auto& x : s) x = std::tolower(x); }

  class DataBlock
  {
  public:
    struct BadDataBlockAccess : cosmosis::Exception { }; // used for exceptions.

    // All memory management functions are compiler generated.

    // Return true if the datablock has a value in the given
    // section with the given name, and false otherwise.
    bool has_val(std::string section,
                             std::string name) const;

    // Return -1 if no parameter of the given name in the given section
    // is found, or if the parameter is not an array. Return -2 if the
    // length of the array is larger than MAXINT. Otherwise, return the
    // length of the array.
    int get_size(std::string section,
                 std::string name) const;

  // Get the type, if any, of the named object.
  // The types are enumerated in 
  // Returns DBS_SUCCESS if found.
  DATABLOCK_STATUS get_type(std::string section,
                            std::string name,
                            datablock_type_t &t) const;


    // get functions return the status, and set the value of their
    // output argument only upon success.
    template <class T>
    DATABLOCK_STATUS get_val(std::string section,
                             std::string name,
                             T& val);

    template <class T>
    DATABLOCK_STATUS get_val(std::string section,
                             std::string name,
                             T const& def,
                             T& val);

    // put and replace functions return the status of the or
    // replace. They modify the state of the object only on success.
    // put requires that there is not already a value with the given
    // name in the given section.
    template <class T>
    DATABLOCK_STATUS put_val(std::string section,
                             std::string name,
                             T const& val);

    // replace requires that there is already a value with the given
    // name and of the same type in the given section.
    template <class T>
    DATABLOCK_STATUS replace_val(std::string section,
                                 std::string name,
                                 T const& val);

    // Return true if the DataBlock has a section with the given name.
    bool has_section(std::string name) const;

    DATABLOCK_STATUS 
    delete_section(std::string section);

    // Return the number of sections in this DataBlock.
    std::size_t num_sections() const;

    // Get the number of values in a named section
    int num_values(std::string const& section) const;

    // Return the name of the i'th section. Throws BadDataBlockAccess if
    // the index is out-of-range.
    std::string const& section_name(std::size_t i) const;

    // Return the name of the value in the given section and position
    // in that section.  Specify section either by number or name
    std::string const& value_name(std::string section, int j) const;
    std::string const& value_name(int i, int j) const;

    // Remove all the sections.
    void clear();

    // The view functions provide readonly access to the data in
    // DataBlock without copying the data. The reference returned by a
    // call to view is invalidated if any replace function is called for
    // the same section and name. Throws BadDataBlockAccess if the
    // section can't be found, BadSection access if the name can't be
    // found, or BadEntry if the contained value is of the wrong type.
    template <class T>
    T const& view(std::string section, std::string name) const;

    void print_log();
    void report_failures(std::ostream &output);

  private:
    std::map<std::string, Section> sections_;
    std::vector<log_entry> access_log_;

    void log_access(const std::string &log_type, const std::string &section, const std::string &name, const std::type_info &type);
  };
}

// Implementation details below.

template <class T>
DATABLOCK_STATUS
cosmosis::DataBlock::get_val(std::string section,
                             std::string name,
                             T& val)
{
  downcase(section); downcase(name);
  auto isec = sections_.find(section);
  if (isec == sections_.end()) {
    log_access(BLOCK_LOG_READ_FAIL, section, name, typeid(val));
    return DBS_SECTION_NOT_FOUND;
  }
  DATABLOCK_STATUS status = isec->second.get_val(name, val);
  if (status==DBS_SUCCESS) log_access(BLOCK_LOG_READ, section, name, typeid(val));
  else log_access(BLOCK_LOG_READ_FAIL, section, name, typeid(val));
  return status;
}

template <class T>
DATABLOCK_STATUS
cosmosis::DataBlock::get_val(std::string section,
                             std::string name,
                             T const& def,
                             T& val)
{
  downcase(section); downcase(name);
  auto isec = sections_.find(section);
  if (isec == sections_.end())
    {
      val = def;
      log_access(BLOCK_LOG_READ_DEFAULT, section, name, typeid(val));
      return DBS_SUCCESS;
    }
  log_access(BLOCK_LOG_READ, section, name, typeid(val));
  DATABLOCK_STATUS status = isec->second.get_val(name, def, val);
  if (status==DBS_SUCCESS) log_access(BLOCK_LOG_READ, section, name, typeid(val));
  else log_access(BLOCK_LOG_READ_FAIL, section, name, typeid(val));
  return status;
}

template <class T>
DATABLOCK_STATUS
cosmosis::DataBlock::put_val(std::string section,
                             std::string name,
                             T const& val)
{
  downcase(section); downcase(name);
  auto& sec = sections_[section]; // create one if needed
  DATABLOCK_STATUS status = sec.put_val(name, val);
  if (status==DBS_SUCCESS){
    log_access(BLOCK_LOG_WRITE, section, name, typeid(val));
  }
  else{
    log_access(BLOCK_LOG_WRITE_FAIL, section, name, typeid(val));
  }
  
  return status;
}

template <class T>
DATABLOCK_STATUS
cosmosis::DataBlock::replace_val(std::string section,
                                 std::string name,
                                 T const& val)
{
  downcase(section); downcase(name);
  auto isec = sections_.find(section);
  if (isec == sections_.end()) return DBS_SECTION_NOT_FOUND;
  DATABLOCK_STATUS status = isec->second.replace_val(name, val);
  if (status==DBS_SUCCESS){
    log_access(BLOCK_LOG_REPLACE, section, name, typeid(val));
  }
  else{
    log_access(BLOCK_LOG_REPLACE_FAIL, section, name, typeid(val));
  }

  return status;
}

template <class T>
T const&
cosmosis::DataBlock::view(std::string section, std::string name) const
{
  downcase(section); downcase(name);
  auto isec = sections_.find(section);
  if (isec == sections_.end()) throw BadDataBlockAccess();
  return isec->second.view<T>(name);
}


#endif
