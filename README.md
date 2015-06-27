# DeepFlags
*A library to simplify command-line flag interfaces*

DeepFlags is a library that simplifies designing command-line APIs by providing
classes to define flags and methods to parse and document them. The purpose of
DeepFlags is similar to that of Google's [GFlags](http://gflags.github.io/gflags/),
but with the intention of being used for more complicated command-line syntaxes.

## Multiple values

Most basically, consider cases where a flag needs to accept multiple values. The
GFlags idiom is to accept a string, and then explode that string on commas. The
example used on the GFlags main page is `--languages=english,french`. While this
makes sense for simple names, it loses its luster for more complicated values,
such as paths. Even with short relative paths, such as
`--input=input/file1.txt,input/file2.txt`,
the command line is hard to parse visually and seems to be begging for trouble.

Therefore, while DeepFlags obviously supports a string definition for `--input`,
eg,

```C++
  Flags::Flag<string> inputFiles = Flags::flag(this, "input", 'i')
      .description("A comma-separated list of input files.");
```

it just as easily supports automating this process:

```C++
  Flags::Flag<vector<string>> inputFiles = Flags::flag(this, "input", 'i')
      .description("Specifies an input file.");
```

Differing simply by a wrapping `vector<>`, the above will allow the user to
specify multiple files, either by passing multiple values after the flag, eg,
`--input "input/file1.txt" "input/file2.txt"`, or by naming the flag twice, eg,
`--input="input/file1.txt" --input="input/file2.txt"`. The two cases may be used
interchangeably.

To more strictly control how additional values can be specified, you may use
`Flag<Sequential<string>>` to accept only the former, or `Flag<Repeated<string>>`
to accept only the latter.

And this is just the tip of the iceberg.

## Groups of values

DeepFlags works on the principle that flags are grouped into `FlagGroup` classes.
Individually, a `FlagGroup` can be treated as a complete program flag interpreter.
The basic interface of DeepFlags is therefore as such:

```C++
struct FoobarFlags : Flags::FlagGroup {
  Flags::Flag<std::string> name = Flags::flag(this, "name", 'n')
      .description("Gives the name of this entity");
  Flags::Flag<int> count = Flags::flag(this, "count", 'c')
      .description("Gives the number of this entity to create");
};
```

You can then instantiate and use `FoobarFlags` as your main flags class, like so:

```C++
int main(int argc, char* argv[]) {
  FoobarFlags flags;
  flags.printHelp(std::cout);
  flags.parseArgs(argc, argv);
}
```

The above code prints nicely-formatted help text to stdout, and then parses the
given command-line flags, automatically populating the flag group as arguments
are given.

Flag groups do, however, have a more versatile purpose. By accepting flag
arguments in its constructor, the flag group can itself be used as a class:

```C++
struct FoobarFlagGroup : Flags::FlagGroup {
  Flags::Flag<std::string> name = Flags::flag(this, "name", 'n')
      .description("Gives the name of this entity");
  Flags::Flag<int> count = Flags::flag(this, "count", 'c')
      .description("Gives the number of this entity to create");

  FoobarFlagGroup(CtorArgs args): FlagGroup(args) {}
};
```

The `FoobarFlagGroup` defined above can then be used as a flag on its own. This
is particularly useful when groups of flags need repeated: it is perfectly valid
to declare a `Flag<vector<FoobarFlagGroup>>` and construct it with a name and
description.

This allows for complicated flag syntaxes to be built. For example, it allows
associating switches and attributes with individual occurrences of a flag.

## Details and Examples

In the enclosed example, a more complicated flag group is used to reprsent files
to be displayed. The group is defined like so:

```C++
struct DisplayFile : Flags::FlagGroup {
  Flags::Flag<std::string> file = Flags::flag(this, "file", 'f')
      .description("Specifies the file to read (reads from stdin by default).");
  Flags::Flag<std::string> label = Flags::flag(this, "label", 'l')
      .description("Assigns a label to this file's tab.");
  Flags::Flag<std::vector<int>> bookmarks = Flags::flag(this, "bookmark", 'b')
      .description("Gives the number of this entity to create.");
  Flags::Switch createIfMissing = Flags::flag(this, 'p')
      .description("Denotes that if this file does not exist, it should be"
          " created. If bookmarks are specified, the file will be sized to"
          " contain the largest bookmark.");

  DisplayFile(CtorArgs args): FlagGroup(args) {}
};
```

The group is then instantiated as `Flags::Flag<Flags::Repeated<DisplayFile>>` in
the main flag group used for interacting with the DeepFlags API. The result is a
command line which accepts input similar to this:

```
FlagsExample \
    --display -f a.txt -l "File 1" -b 1 -b 3 5 -p -b 7 \
    -D --file b.txt -l "File 2" --bookmark=1 3 5 \
    -D --file "c.txt" --label "File 3" -pb 9 10 35
```

The above command will be interpreted as a directive to "display" three files,
named "a.txt," "b.txt," and "c.txt," labeling them "File 1," "File 2," and
"File 3," respectively, and bookmarking a few lines in each of them. Files one
and three are flagged to be created if they are not found.

Notice that the single-character names 'p' and 'b' have been strung together.
This is valid for any kind of flag, but flags accepting values will not receive
any if they are used in the middle of such a chain. For that reason, `-bp 10`
would not work in this example, as the "10" will not be handled by the bookmark
flag, and the `-p` switch does not accept values. Since the bookmark flag is a
vector, however, it can accept zero parameters, and so specifying `-bp` with no
following values is valid.

Notice also that short names and long names can be used interchangeably, and
that the `--flag=value` format and `--flag value` formats can be as well (even
in vector flags). This is supported behavior. However, since stringing short
flag names together is supported, the use of, eg, `-l="label"` is not presently
valid.

## To-do

There's still a laundry list of missing features. The most important of these,
in my opinion, are as follows:

1. **Flag Validation.** There's a way to label a flag as required, but not a
   way to actually ensure that required fields were passed.
2. **Default values.** This is a bit tricky, and might require a small API
   change, so getting it done quickly is rather important.
3. **Bash completion.** Google Flags supports full-blown bash completion.
   I would like to enable building a bash completion module from flag
   descriptors (or creating code that emits a bash completion script).
4. **Porting.** This library depends very heavily on template specialization,
   which it uses to give special treatment to various flag types. While this
   does not directly translate to any other language I intend supporting (Rust
   being a possible exception), other languages provide their own mechanisms by
   which the same effect can be achieved (such as reflection in Java or the RTTI
   provided by scripting languages such as Python and JavaScript).

## License

While the license is currently GPL, I am open to the idea of a more permissive
license. I have considered Apache 2. Contact me if you require relicensing.
