
ZipArchive::ZipArchive(const uint8_t *data, size_t size) : m_size(size) {
  // own a copy of the data:
  m_data = (uint8_t *) MZ_MALLOC(m_size);
  memcpy(m_data, data, m_size);

  mz_zip_zero_struct(&m_zar);
  if (!mz_zip_reader_init_mem(&m_zar, (const void *) m_data, m_size, 0)) {
    _throw("mz_zip_reader_init_mem");
  }
}

ZipArchive::~ZipArchive() {
  if (!mz_zip_reader_end(&m_zar)) {
    _throw("mz_zip_reader_end");
  }

  // free the data copy:
  MZ_FREE(m_data);
  m_data = nullptr;
}

bool ZipArchive::file_locate(const char *pFilename, uint32_t* o_index) {
  if (!mz_zip_reader_locate_file_v2(&m_zar, pFilename, nullptr, 0, o_index)) {
    return _throw("mz_zip_reader_locate_file_v2");
  }

  return true;
}

bool ZipArchive::file_size(uint32_t i_index, uint64_t *o_size) {
  mz_zip_archive_file_stat s;
  if (!mz_zip_reader_file_stat(&m_zar, (mz_uint) i_index, &s)) {
    return _throw("mz_zip_reader_file_stat");
  }

  *o_size = s.m_uncomp_size;
  return true;
}

bool ZipArchive::file_extract(uint32_t i_index, void *o_data, size_t i_size) {
  if (!mz_zip_reader_extract_to_mem(&m_zar, (mz_uint) i_index, o_data, i_size, 0)) {
    return _throw("mz_zip_reader_extract_to_mem");
  }

  return true;
}

bool ZipArchive::_throw(const std::string &name) {
  mz_zip_error err = mz_zip_get_last_error(&m_zar);
  if (err == MZ_ZIP_NO_ERROR)
    return true;

  std::string errtxt(name);
  errtxt.append(": ");
  errtxt.append(mz_zip_get_error_string(err));
  m_err = WASMError("", name, mz_zip_get_error_string(err));
  return false;
}

WASMError ZipArchive::last_error() const {
  return m_err;
}
