**Added:**

* Added command line option `--group-uncommented` to add uncommented members to
  the group of the preceding member, e.g., here both operators are put in the
  same group `Arithmetic` without having to explicitly mention the group for
  the second line.
  ```cpp
  struct S {
      /// \group Arithmetic
      S& operator+=(const S&);
      S& operator-=(const S&);
  }
  ```
**Fixed:**

* Added a missing mutex lock in the comment parser.
