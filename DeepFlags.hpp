/**
 * @file DeepFlags.hpp
 * @section License
 *
 * Copyright (C) 2015 Josh Ventura
 *
 * DeepFlags is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 * 
 * DeepFlags is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * DeepFlags.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FLAGS_h
#define FLAGS_h

#include <map>
#include <queue>
#include <vector>
#include <string>
#include <iterator>
#include <initializer_list>
#include <type_traits>
#include <limits>
#include <cctype>
#include <cstdint>
#include <cstdio>

namespace Flags {
  class FlagGroup;
  
  class FlagProperties {
    const std::string flagName;
    const char shortName;
    const std::string valueName;
    const bool greedy;
    const bool reentrant;
   
   public:
    FlagProperties(std::string flagNameOrEmpty, char shortNameOrZero,
        std::string valueNameOrEmpty, bool isGreedy, bool isReentrant):
            flagName(flagNameOrEmpty), shortName(shortNameOrZero),
            valueName(valueNameOrEmpty),
            greedy(isGreedy),
            reentrant(isReentrant) {}
    
    bool hasShortName() const { return shortName; }
    bool hasLongName()  const { return flagName.length(); }
    bool hasValueName() const { return valueName.length(); }
    
    bool hasAnyName() const { return hasShortName() || hasLongName(); }
    
    char getShortName()        const { return shortName; }
    std::string getLongName()  const { return flagName;  }
    std::string getValueName() const { return valueName; }
    
    bool isRepeatable() const { return reentrant; }
    bool acceptsMultipleValues() const { return greedy; }
    
    std::string listFlagNames() const {
      if (hasLongName()) {
        if (hasShortName()) {
          char rbuf[5] = ", -?";
          rbuf[3] = getShortName();
          return "--" + getLongName() + rbuf;
        }
        return "--" + getLongName();
      }
      if (hasShortName()) {
        char res[3] = "-?";
        res[1] = getShortName();
        return res;
      }
      return "";
    }
  };
  
  class HelpPrinter {
   public:
    virtual void writeBlock(std::string textString) = 0;
    virtual void enterFlag(FlagProperties) = 0;
    virtual void leaveFlag() = 0;
    
    virtual ~HelpPrinter() {}
  };
  
  class BasicHelpPrinter: public HelpPrinter {
    const size_t consoleWidth;

    std::ostream &stream;
    bool inFlag = false;
    int indent;
    
    static constexpr size_t cstrlen(const char* str) {
      return *str? 1 + cstrlen(str + 1) : 0;
    }
    
    static size_t getConsoleWidth() {
      int res;
      char *columns = getenv("COLUMNS");
      return (columns && isdigit(*columns) && (res = atoi(columns)))? res : 80;
    }
    
    inline void writeIndentation() const {
      constexpr const char *indstr = "                ";
      constexpr size_t indbufsiz = cstrlen(indstr);
      static_assert(indbufsiz > 0, "Indent buffer size invalid.");
      
      unsigned toPrint = indent;
      while (toPrint > indbufsiz) {
        toPrint -= indbufsiz;
        stream.write(indstr, indbufsiz);
      }
      stream.write(indstr, toPrint);
    }
    
    void writeIndentedBlock(std::string textString) {
      const char* const text = textString.c_str();
      const size_t tlen = textString.length();
      
      writeIndentation();
      const size_t workingSpace = consoleWidth - indent;
      for (size_t i = 0, lineStart = i; i < tlen; ) {
        const size_t indentStart = i;
        while (isspace(text[i]) && ++i < tlen);
        if (i >= tlen) {
          break;
        }
        
        const size_t wordStart = i;
        while (!isspace(text[++i]) && i < tlen);
        const size_t wlen = i - wordStart;
        
        if (i - lineStart > workingSpace) {
          stream << std::endl;
          writeIndentation();
          lineStart = wordStart;
        } else {
          stream.write(text + indentStart, wordStart - indentStart);
        }
        
        stream.write(text + wordStart, wlen);
      }
      
      stream << std::endl << std::endl;
    }
    
   public:
    BasicHelpPrinter(std::ostream &ostream):
        consoleWidth(getConsoleWidth()), stream(ostream), indent(0) {}
    
    void enterFlag(FlagProperties props) override {
      if (props.hasAnyName()) {
        writeIndentation();
        stream << "\x1B[1m";
        writeFlagHeader(stream, props);
        stream << "\x1B[0m";
        stream << std::endl << std::endl;
        inFlag = true;
      }
      if (inFlag) {
        indent += 2;
      } else {
        inFlag = true;
      }
    }
    void writeBlock(std::string block) override {
      writeIndentedBlock(block);
    }
    void leaveFlag() override {
      indent = (indent > 2) ? indent - 2 : 0;
    }
    
    static void writeFlagHeader(std::ostream &sstream, FlagProperties props) {
      bool written = false;
      if (props.hasAnyName()) {
        sstream << props.listFlagNames();
        written = true;
      }
      
      const bool greedy = props.acceptsMultipleValues();
      const bool reentrant = props.isRepeatable();
      if (props.hasValueName()) {
        if (written) {
          sstream << " ";
        } else {
          sstream << "[";
        }
        
        sstream << props.getValueName();
        if (greedy) {
          sstream << " [" << props.getValueName()
                 << " [" << props.getValueName() << "...]]";
        }
        
        if (!written) {
          sstream << "]";
          written = true;
        }
        
        if (reentrant && written) {
          sstream << " (Flag can " << (greedy? "also be" : "be") << " repeated)";
        }
      } else {
        if (reentrant) {
          if (written) {
            sstream << " ";
          } else {
            written = true;
          }
          sstream << "[Repeatable]";
          written = true;
        }
        if (greedy) {
          if (written) {
            sstream << " ";
          } else {
            written = true;
          }
          sstream << "[Accepts multiple values]";
        }
      }
    }
    
    ~BasicHelpPrinter() override {}
  };
  
  namespace Internal {
    using std::map;
    using std::string;
    using std::queue;
    
    struct CtorArgs {
      FlagGroup *_group;
      string _longName;
      char _shortName;
      
      string _description;
      string _valuename;
      bool _required = false;
      
      CtorArgs(FlagGroup *group, string longName):
          _group(group), _longName(longName), _shortName(0) {}
      
      CtorArgs(FlagGroup *group, string longName, char shortName):
          _group(group), _longName(longName), _shortName(shortName) {}
      
      CtorArgs(FlagGroup *group, char shortName):
          _group(group), _longName(), _shortName(shortName) {}
      
      // ---------------------------------------------------------------
      // Builder methods
      // ---------------------------------------------------------------
      
      CtorArgs &description(string desc) {
        _description = desc;
        return *this;
      }
      
      CtorArgs &valueName(string desc) {
        _valuename = desc;
        return *this;
      }
      
      CtorArgs &required() {
        _required = true;
        return *this;
      }
      
      // ---------------------------------------------------------------
          
      CtorArgs():
          _group(nullptr), _longName(), _shortName(0) {}
    };
    
    class ArgReader {
      const unsigned argc;
      const char *const *const argv;
      
      char charKey;
      std::string key;
      std::string value;
      
      bool flagAbsent = true;
      bool flagIsCharacter = false;
      bool valueSpecified = false;
      bool clearedOut = false;
      unsigned position = 0;
      
      std::queue<char> charFlags;
      
     public:
      bool atEnd() const {
        return clearedOut;
      }
      
      bool hasMoreArguments() const {
        return position < argc;
      }
      
      void parseNextArg() {
        flagAbsent = false;
        valueSpecified = false;
        flagIsCharacter = false;
        value.clear();
        charKey = 0;
        
        if (charFlags.size()) {
          flagIsCharacter = true;
          charKey = charFlags.front();
          charFlags.pop();
          return;
        }
        
        if (position >= argc || ++position >= argc) {
          key.clear();
          clearedOut = true;
          return;
        }
        
        const char *curArg = argv[position];
        if (*curArg != '-') {
          key.clear();
          value = curArg;
          flagAbsent = true;
          valueSpecified = true;
          return;
        }
        if (curArg[1] == '-') {
          for (size_t i = 2; curArg[i]; ++i) {
            if (curArg[i] == '=') {
              key = string(curArg + 2, curArg + i);
              value = string(curArg + i + 1);
              valueSpecified = true;
              return;
            }
          }
          key = curArg + 2;
          return;
        }
        
        flagIsCharacter = true;
        key.clear();
        charKey = curArg[1];
        for (size_t i = 2; curArg[i]; ++i) {
          charFlags.push(curArg[i]);
        }
      }
      
      /** Extract and return the next argument as a raw value. */
      string nextRawArgument() {
        flagAbsent = true;
        valueSpecified = true;
        flagIsCharacter = false;
        key.clear();
        return value = argv[++position];
      }
      
      bool hasValue() const {
        return valueSpecified;
      }
      
      string getValue() const {
        return value;
      }
      
      bool hasAnyFlag() const {
        return !flagAbsent;
      }
      
      bool hasLongFlag() const {
        return !flagAbsent && !flagIsCharacter;
      }
      
      bool hasShortFlag() const {
        return !flagAbsent && flagIsCharacter;
      }
      
      string getLongFlag() const {
        return key;
      }
      
      char getShortFlag() const {
        return charKey;
      }
      
      string quotedFlagName() const {
        if (hasLongFlag()) {
          return "\"" + getLongFlag() + "\"";
        }
        if (hasShortFlag()) {
          char res[4] = "'?'";
          res[1] = getShortFlag();
          return res;
        }
        return "<Unspecified>";
      }
      
      unsigned tell() const {
        return position;
      }
      
      ArgReader(int _argc, const char *const *const _argv):
          argc(_argc < 0 ? 0 : _argc), argv(_argv) {}
    };
    
    class FlagBase {
      const string _name;
      const char _shortname;
      bool _required = false;
      
      const string _description;
      const string _valuename;
      
      inline void addThisTo(FlagGroup *group);
    
     protected:
      typedef Internal::CtorArgs CtorArgs;
      
      /// Circumvents the fact that protected members can only be called on
      /// pointers to the caller class.
      static inline bool invokeParse(FlagBase *flag, ArgReader &argReader) {
        return flag->parseArgsR(argReader);
      }
      
      /// Circumvents the fact that protected members can only be called on
      /// pointers to the caller class.
      static bool pHasFlag(const FlagBase &fb, string flag) {
        return fb.hasFlag(flag);
      }
      
      /// Circumvents the fact that protected members can only be called on
      /// pointers to the caller class.
      static bool pHasFlag(const FlagBase &fb, char flag) {
        return fb.hasFlag(flag);
      }
      
      /// Circumvents the fact that protected members can only be called on
      /// pointers to the caller class.
      static bool pAtCapacity(const FlagBase &fb) {
        return fb.atCapacity();
      }
      
      /**
       * Parses as many values as possible from the given stream.
       * @return true on success, false if an error occurred
       * @note this function can return success even if no input is parsed
       */
      virtual bool parseArgsR(ArgReader& argReader) = 0;
      
      /** Returns false iff this flag can receieve an additional value. */
      virtual bool atCapacity() const = 0;
      
      /**
       * Returns true if this flag has anything to do with the given flag name,
       * or false otherwise.
       */
      virtual bool hasFlag(string name) const = 0;
      
      /**
       * Returns true if this flag has anything to do with the given flag name,
       * or false otherwise.
       */
      virtual bool hasFlag(char name) const = 0;
      
      /**
       * Print help text for this flag to the given stream. Prefix the given
       * indentation.
       */
      virtual void printHelp(HelpPrinter &printer) const {
        printer.enterFlag(makeProps());
        if (hasDescription()) {
          printer.writeBlock(getDescription());
        }
        printer.leaveFlag();
      }
      
      FlagProperties makeProps(
          bool greedy = false, bool reentrant = false) const {
        return FlagProperties(_name, _shortname, _valuename, greedy, reentrant);
      }
      
      static void printHelpR(const FlagBase *flag, HelpPrinter &printer) {
        flag->printHelp(printer);
      }
      
      /**
       * Perform the neccessary steps to ensure the contents of this flag will
       * be parsed if the flags are present. Normally this is the same as a call
       * to `addThisTo()`. An example of where this is not the case would be an
       * anonymous flag group, which adds its contents to the given group.
       */
      virtual bool addSelfTo(FlagGroup *group) {
        addThisTo(group);
        return true;
      }
      
      class Instantiator {
       public:
        template<class T> static T instantiate(CtorArgs args) {
          return T(args);
        }
      };
      
      CtorArgs getCtorArgs(FlagGroup *group) const {
        return CtorArgs(group, _name, _shortname);
      }
      
      FlagBase(CtorArgs construct):
          _name(construct._longName), _shortname(construct._shortName),
          _description(construct._description),
          _valuename(construct._valuename) {
        if (construct._group) {
          addThisTo(construct._group);
        }
      }
      
     public:

      bool hasShortName() const {
        return _shortname;
      }

      char getShortName() const {
        return _shortname;
      }

      bool hasLongName() const {
        return _name.length();
      }

      string getLongName() const {
        return _name;
      }

      bool hasAnyName() const {
        return _name.length() || _shortname;
      }
      
      bool hasDescription() const {
        return _description.length();
      }
      
      string getDescription() const {
        return _description;
      }
      
      bool hasValueName() const {
        return _valuename.length();
      }
      
      string getValueName() const {
        return _valuename;
      }

      bool parseArgs(int argc, const char* const* const argv) {
        if (argc < 2) {
          return true; 
        }
        ArgReader argReader(argc, argv);
        argReader.parseNextArg();
        if (!parseArgsR(argReader)) {
          return false;
        }
        if (!argReader.atEnd()) {
          if (argReader.hasLongFlag()) {
            fprintf(stderr, "Unexpected flag \"%s\"\n",
                argReader.getLongFlag().c_str());
          } else if (argReader.hasShortFlag()) {
            fprintf(stderr, "Unexpected flag '%c'\n",
                argReader.getShortFlag());
          } else if (argReader.hasValue()) {
            fprintf(stderr, "Expected flag name, but got \"%s\"\n",
                argReader.getValue().c_str());
          } else {
            fputs("Internal error: "
                "Not all arguments were read and argument reader is not sane.",
                stderr);
          }
          return false;
        }
        return true;
      }

      virtual ~FlagBase() {}
    };
    
    static inline map<string, bool> _boolNames() {
      map<string, bool> res;
      res["1"] = true;
      res["on"] = true;
      res["yes"] = true;
      res["true"] = true;
      res["0"] = false;
      res["no"] = false;
      res["off"] = false;
      res["false"] = false;
      return res;
    }
    
    static map<string, bool> boolNames = _boolNames();
    
    static string lower(string x) {
      for (size_t i = 0; i < x.length(); ++i) {
        x[i] = std::tolower(x[i]);
      }
      return x;
    }
    
    template<typename T> class ParseType;
    
    template<> struct ParseType<bool> {
      bool error = false;
      bool value = false;
      ParseType(string val) {
        auto valit = Internal::boolNames.find(Internal::lower(val));
        if (valit == Internal::boolNames.end()) {
          error = true;
        } else {
          value = valit->second;
        }
      }
    };
    
    template<> struct ParseType<char> {
      bool error = false;
      char value = 0;
      ParseType(string val) {
        if (val.length() != 1) {
          error = true;
        } else {
          value = val[0];
        }
      }
    };
    
    template<typename T, typename BT, BT (*parse)(const string&, size_t*, int)>
    struct ParseIntType {
      bool error = false;
      T value = 0;
      ParseIntType(string val) {
        try {
          BT i = parse(val, nullptr, 0);
          if (i >= std::numeric_limits<T>::min()
              && i <= std::numeric_limits<T>::max()) {
            value = i;
            return;
          }
        } catch (...) {}
        error = true;
      }
    };
    
#   define df_internal_DEFINE_PRIM_PARSER(T, BT, IntParser) \
    template<> struct ParseType<T>: ParseIntType<T, BT, IntParser> { \
      ParseType(string val): ParseIntType(val) {} \
    }
    
    df_internal_DEFINE_PRIM_PARSER(int8_t,  long long, std::stoll);
    df_internal_DEFINE_PRIM_PARSER(int16_t, long long, std::stoll);
    df_internal_DEFINE_PRIM_PARSER(int32_t, long long, std::stoll);
    df_internal_DEFINE_PRIM_PARSER(int64_t, long long, std::stoll);
    
    df_internal_DEFINE_PRIM_PARSER(uint8_t,  unsigned long long, std::stoull);
    df_internal_DEFINE_PRIM_PARSER(uint16_t, unsigned long long, std::stoull);
    df_internal_DEFINE_PRIM_PARSER(uint32_t, unsigned long long, std::stoull);
    df_internal_DEFINE_PRIM_PARSER(uint64_t, unsigned long long, std::stoull);
    
#   undef df_internal_DEFINE_PRIM_PARSER

    template<typename T> struct ParseFloatType {
      bool error = false;
      T value = 0;
      ParseFloatType(string val) {
        try {
          long double i = std::stold(val, nullptr);
          if (i >= std::numeric_limits<T>::min()
              && i <= std::numeric_limits<T>::max()) {
            value = i;
            return;
          }
        } catch (...) {}
        error = true;
      }
    };
    
    template<> struct ParseType<float>: ParseFloatType<float> {
      ParseType(string val): ParseFloatType(val) {}
    };
    
    template<> struct ParseType<double>: ParseFloatType<double> {
      ParseType(string val): ParseFloatType(val) {}
    };
    
    template<> struct ParseType<long double>: ParseFloatType<long double> {
      ParseType(string val): ParseFloatType(val) {}
    };
    
    template<> struct ParseType<string> {
      bool error = false;
      string value;
      ParseType(string val): value(val) {}
    };
    
    class SingletonFlag: public FlagBase {
     protected:
      virtual bool parse(string rawvalue) = 0;
      
      SingletonFlag(CtorArgs args): FlagBase(args) {}
      
      bool parseArgsR(ArgReader& argReader) final override {
        if (argReader.hasValue()) {
          if (!parse(argReader.getValue())) {
            return false;
          }
          argReader.parseNextArg();
          return true;
        } else if (argReader.hasMoreArguments()) {
          if (!parse(argReader.nextRawArgument())) {
            return false;
          }
          argReader.parseNextArg();
          return true;
        }
        return false;
      }
    };
    
    template<typename P> class PrimitiveFlag: public SingletonFlag {
     public:
      bool present = false;
      P value = P();
      
     protected:
      PrimitiveFlag(CtorArgs args): SingletonFlag(args) {}
      
      bool atCapacity() const override {
        return present;
      }
      
      bool hasFlag(string name) const override {
        return getLongName() == name;
      }
      
      bool hasFlag(char name) const override {
        return getShortName() == name;
      }
      
     public:
      bool parse(string rawvalue) override {
        ParseType<P> parsed(rawvalue);
        if (parsed.error) {
          return false;
        }
        value = parsed.value;
        present = true;
        return true;
      }
    };
    
    /// Wrapper to std::is_base_of to make other code bits more legible.
    template<typename T> constexpr bool isFlag() {
      return std::is_base_of<FlagBase, T>::value;
    }
    
    /**
     * The purpose of this function is to trick the compiler into waiting to
     * evaluate a static_assert until the class is instantiated. Only then will
     * the assertion fail and an understandable compile error be raised.
     */
    template<typename T> constexpr bool assertionConvolution() {
      return false;
    }
  } // namespace Internal

  /// Generic flag class; instantuate with a type to use.
  template<typename T, bool b = Internal::isFlag<T>()> class Flag {
    static_assert(Internal::assertionConvolution<T>(),
        "Attempted to instantiate Flag with unsupported type");
  };
  
  /** Specialization of Flag<T> for when T is also a Flag<> */
  template<typename T, bool b> class Flag<Flag<T, false>, b>:
      public Internal::FlagBase {
    Flag<T> parser;
    
   protected:
    bool parseArgsR(Internal::ArgReader &argReader) final override {
      return parser.parseArgsR(argReader);
    }
     
   public:
    T &value = parser.value;
  };
  
  /// Specialization of Flag<T> for when T is any other kind of Flag (FlagBase)
  template<typename T> class Flag<T, true>: public Internal::FlagBase {
    using FlagBase::parseArgsR;
   protected:
    bool atCapacity() const override {
      return pAtCapacity(value);
    }
    
    bool hasFlag(std::string name) const override {
      return pHasFlag(value, name);
    }
    
    bool hasFlag(char name) const override {
      return pHasFlag(value, name);
    }
    
    bool parseArgsR(Internal::ArgReader &argReader) final override {
      return invokeParse(&value, argReader);
    }
    
    void printHelp(HelpPrinter &printer) const final override {
      printHelpR(&value, printer);
    }
    
   public:
    T value;
     
    Flag(CtorArgs args): FlagBase(args), value(args) {}
  };

  class Switch: public Internal::FlagBase {
    bool parseArgsR(Internal::ArgReader &argReader) final override {
      if (argReader.hasValue()) {
        fprintf(stderr, "Flag %s is a switch and cannot accept a value\n",
            argReader.quotedFlagName().c_str());
        return false;
      }
      present = true;
      argReader.parseNextArg();
      return true;
    }
    
    bool atCapacity() const final override {
      return present;
    }
    
    bool hasFlag(std::string name) const final override {
      return getLongName() == name;
    }
    
    bool hasFlag(char name) const final override {
      return getShortName() == name;
    }
    
   public:
    bool present = false;
    Switch(CtorArgs args): FlagBase(args) {}
  };

# define df_internal_DEFINE_PRIMITIVE_FLAG(T) \
  template<bool b> class Flag<T, b>: public Internal::PrimitiveFlag<T> { \
    friend class FlagBase::Instantiator; \
    \
   public: \
    Flag(CtorArgs args): PrimitiveFlag<T>(args) {} \
  }
  
  df_internal_DEFINE_PRIMITIVE_FLAG(bool);
  df_internal_DEFINE_PRIMITIVE_FLAG(int8_t);
  df_internal_DEFINE_PRIMITIVE_FLAG(int16_t);
  df_internal_DEFINE_PRIMITIVE_FLAG(int32_t);
  df_internal_DEFINE_PRIMITIVE_FLAG(int64_t);
  df_internal_DEFINE_PRIMITIVE_FLAG(uint8_t);
  df_internal_DEFINE_PRIMITIVE_FLAG(uint16_t);
  df_internal_DEFINE_PRIMITIVE_FLAG(uint32_t);
  df_internal_DEFINE_PRIMITIVE_FLAG(uint64_t);
  df_internal_DEFINE_PRIMITIVE_FLAG(float);
  df_internal_DEFINE_PRIMITIVE_FLAG(double);
  df_internal_DEFINE_PRIMITIVE_FLAG(long double);
  df_internal_DEFINE_PRIMITIVE_FLAG(std::string);

#undef df_internal_DEFINE_PRIMITIVE_FLAG

  /*template<typename ... Args>
      class Flag<std::tuple<Args...>>: public Internal::FlagBase {
    typedef std::tuple<Args...> ValueTuple;
    ValueTuple value;

   public:
    Flag(std::initializer_list<string> names, ValueTuple defaults):
        Flag(names[0]), value(defaults) {}
  };*/
  
  
  template<typename T, bool greedy, bool reentrant> class VectorFlag:
      public Internal::FlagBase {
    bool entered = false;
    
    /// Stack a new parser for our type
    Flag<T> newFlag() const {
      return FlagBase::Instantiator::instantiate<Flag<T>>(getCtorArgs(nullptr));
    }
    
    /// Stack a new help printer for our type
    Flag<T> newAnonymousFlag() const {
      return FlagBase::Instantiator::instantiate<Flag<T>>(CtorArgs());
    }
    
    static bool canReenter(
        const Internal::ArgReader &argReader, const FlagBase& flag) {
      if (!reentrant) {
        return false;
      }
      return argReader.hasShortFlag()
          ? pHasFlag(flag, argReader.getShortFlag())
          : pHasFlag(flag, argReader.getLongFlag());
    }
    
   protected:
    bool atCapacity() const final override {
      return entered && !reentrant;
    }
    
    bool hasFlag(std::string name) const final override {
      return getLongName() == name || pHasFlag(newFlag(), name);
    }
    
    bool hasFlag(char name) const final override {
      return getShortName() == name || pHasFlag(newFlag(), name);
    }
    
    void printHelp(HelpPrinter &printer) const override {
      printer.enterFlag(makeProps(greedy, reentrant));
      if (hasDescription()) {
        printer.writeBlock(getDescription());
      }
      auto nflag = newAnonymousFlag();
      printHelpR(&nflag, printer);
      printer.leaveFlag();
    }
    
    bool parseArgsR(Internal::ArgReader &argReader) final override {
      entered = true;
      Flag<T> flag = newFlag();
      size_t position = argReader.tell();
      if (!invokeParse(&flag, argReader)) {
        return false;
      }
      if (position == argReader.tell()) {
        return true;
      }
      
      value.push_back(flag.value);
      if (greedy
          && (!argReader.hasAnyFlag() || canReenter(argReader, flag))) {
        return parseArgsR(argReader);
      }
      return true;
    }
    
   public:
    VectorFlag(CtorArgs args): FlagBase(args) {}
    std::vector<T> value;
  };
  
  template<typename T> class Flag<std::vector<T>, false>:
      public VectorFlag<T, true, true> {
    typedef VectorFlag<T, true, true> Super;

   public:
    Flag(Internal::CtorArgs args): Super(args) {}
  };
  
  template<typename T> class Repeated : public std::vector<T> {};
  template<typename T> class Sequential : public std::vector<T> {};
  
  template<typename T> class Flag<Sequential<T>, false>:
      public VectorFlag<T, true, false> {
    typedef VectorFlag<T, true, false> Super;

   public:
    Flag(Internal::CtorArgs args): Super(args) {}
  };
  
  template<typename T> class Flag<Repeated<T>, false>:
      public VectorFlag<T, false, true> {
    typedef VectorFlag<T, false, true> Super;

   public:
    Flag(Internal::CtorArgs args): Super(args) {}
  };

  class FlagGroup: public Internal::FlagBase {
    std::vector<Internal::FlagBase*> members;
    std::map<std::string, Internal::FlagBase*> membersByLongName;
    std::map<char, Internal::FlagBase*> membersByShortName;
    
   protected:
    bool atCapacity() const final override {
      for (size_t i = 0; i < members.size(); ++i) {
        if (!pAtCapacity(*members[i])) {
          return false;
        }
      }
      return true;
    }
    
    bool hasFlag(std::string name) const final override {
      for (size_t i = 0; i < members.size(); ++i) {
        if (pHasFlag(*members[i], name)) {
          return true;
        }
      }
      return false;
    }
    
    bool hasFlag(char name) const final override {
      for (size_t i = 0; i < members.size(); ++i) {
        if (pHasFlag(*members[i], name)) {
          return true;
        }
      }
      return false;
    }
    
    bool parseArgsR(Internal::ArgReader &argReader) final override {
      if (argReader.hasLongFlag() == argReader.hasShortFlag()) {
        if (argReader.hasLongFlag()) {
          fprintf(stderr,
              "Internal error: concurrently read flags \"%s\", '%c'\n",
              argReader.getLongFlag().c_str(), argReader.getShortFlag());
        } else if (argReader.hasValue()) {
          fprintf(stderr, "Expected flag name, got \"%s\"\n",
              argReader.getValue().c_str());
        } else {
          fputs("Internal error: flag parser invoked with no data", stderr);
        }
        return false;
      }
      
      if ((hasLongName() && argReader.hasLongFlag()
              && getLongName() == argReader.getLongFlag())
          || (hasShortName() && argReader.hasShortFlag()
              && getShortName() == argReader.getShortFlag())) {
        argReader.parseNextArg();
      }
      
      while (!argReader.atEnd()) {
        if (argReader.hasLongFlag()) {
          auto flag = membersByLongName.find(argReader.getLongFlag());
          if (flag == membersByLongName.end() || pAtCapacity(*flag->second)) {
            return true;
          }
          unsigned pos = argReader.tell();
          if (!invokeParse(flag->second, argReader)) {
            return false;
          }
          if (argReader.tell() == pos) {
            return true;
          }
        } else {
          auto flag = membersByShortName.find(argReader.getShortFlag());
          if (flag == membersByShortName.end() || pAtCapacity(*flag->second)) {
            return true;
          }
          if (!invokeParse(flag->second, argReader)) {
            return false;
          }
        }
      }
      return true;
    }
    
   public:
    void addFlag(FlagBase *flag) {
      members.push_back(flag);
      if (flag->hasLongName()) {
        membersByLongName[flag->getLongName()] = flag;
      }
      if (flag->hasShortName()) {
        membersByShortName[flag->getShortName()] = flag;
      }
    }
    
    void printHelp(std::ostream &stream) const {
      BasicHelpPrinter printer(stream);
      printHelp(printer);
    }
    
    void printHelp(HelpPrinter &printer) const override {
      printer.enterFlag(makeProps());
      for (Internal::FlagBase *flag : members) {
        printHelpR(flag, printer);
      }
      printer.leaveFlag();
    }
    
    FlagGroup(CtorArgs args): FlagBase(args) {}
    FlagGroup(): FlagBase(CtorArgs()) {}
  };
  
  inline void Internal::FlagBase::addThisTo(FlagGroup *group) {
    group->addFlag(this);
  }
  
  inline Internal::CtorArgs flag(FlagGroup *group, std::string name) {
    return Internal::CtorArgs(group, name);
  }
  
  inline Internal::CtorArgs flag(
      FlagGroup *group, std::string name, char shortname) {
    return Internal::CtorArgs(group, name, shortname);
  }
  
  inline Internal::CtorArgs flag(FlagGroup *group, char shortname) {
    return Internal::CtorArgs(group, shortname);
  }
}

#endif // FLAGS_h
