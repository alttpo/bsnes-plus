#pragma once

struct ZipArchive {
  explicit ZipArchive(const uint8_t *data, size_t size);
  ~ZipArchive();

  bool file_locate(const char *pFilename, uint32_t* o_index);
  bool file_size(uint32_t i_index, uint64_t* o_size);
  bool file_extract(uint32_t i_index, void *o_data, size_t i_size);

  bool _throw(const std::string& name);
  WASMError last_error() const;

private:
  mz_zip_archive m_zar;
  uint8_t* m_data;
  size_t m_size;

  WASMError m_err;
};
