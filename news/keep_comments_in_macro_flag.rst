**Fixed:**

* Failure to parse files that use Boost ASIO and others that depend on Boost.MPL, by enabling cppast `remove_comments_in_macro` feature by default and providing a CLI flag to disable it.
