#pragma once

struct ZipArchive {
  explicit ZipArchive(const uint8_t *data, size_t size);
  ~ZipArchive();

  struct FileHandle {
    explicit FileHandle(int index);
    operator bool() const;
    explicit operator int() const;

    int m_index;
  };

  FileHandle file_locate(const char *pFilename);
  bool file_size(FileHandle fh, uint64_t* o_size);
  bool file_extract(FileHandle fh, void *o_data, size_t i_size);

  void check_error(const std::string& name);

private:
  mz_zip_archive m_zar;
  uint8_t* m_data;
  size_t m_size;
};
