**Added:**

* `--comment.free_file_comments` flag so that comments that cannot be
  associated to any entity are automatically considered to be referring to the
  entire file even if there's no `\file` command. (The default is still the old
  behaviour. Such comments are ignored with a warning.)
