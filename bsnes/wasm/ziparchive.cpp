
ZipArchive::ZipArchive(const uint8_t *data, size_t size) : m_size(size) {
  // own a copy of the data:
  m_data = (uint8_t *) MZ_MALLOC(m_size);
  memcpy(m_data, data, m_size);

  mz_zip_zero_struct(&m_zar);
  if (!mz_zip_reader_init_mem(&m_zar, (const void *) m_data, m_size, 0)) {
    check_error("mz_zip_reader_init_mem");
  }
}

ZipArchive::~ZipArchive() {
  if (!mz_zip_reader_end(&m_zar)) {
    check_error("mz_zip_reader_end");
  }

  // free the data copy:
  MZ_FREE(m_data);
  m_data = nullptr;
}

ZipArchive::FileHandle::FileHandle(int index) : m_index(index) {}

ZipArchive::FileHandle::operator bool() const {
  return m_index >= 0;
}

ZipArchive::FileHandle::operator int() const {
  return m_index;
};

ZipArchive::FileHandle ZipArchive::file_locate(const char *pFilename) {
  mz_uint32 h;
  if (!mz_zip_reader_locate_file_v2(&m_zar, pFilename, nullptr, 0, &h)) {
    check_error("mz_zip_reader_locate_file_v2");
  }

  return ZipArchive::FileHandle(h);
}

bool ZipArchive::file_size(ZipArchive::FileHandle fh, uint64_t *o_size) {
  if (!fh) {
    return false;
  }

  mz_zip_archive_file_stat s;
  if (!mz_zip_reader_file_stat(&m_zar, (mz_uint) (int) fh, &s)) {
    check_error("mz_zip_reader_file_stat");
  }

  *o_size = s.m_uncomp_size;
  return true;
}

bool ZipArchive::file_extract(ZipArchive::FileHandle fh, void *o_data, size_t i_size) {
  if (!fh) {
    return false;
  }

  if (!mz_zip_reader_extract_to_mem(&m_zar, (mz_uint) (int) fh, o_data, i_size, 0)) {
    check_error("mz_zip_reader_extract_to_mem");
  }

  return true;
}

void ZipArchive::check_error(const std::string &name) {
  mz_zip_error err = mz_zip_get_last_error(&m_zar);
  if (err != MZ_ZIP_NO_ERROR) {
    std::string errtxt(name);
    errtxt.append(": ");
    errtxt.append(mz_zip_get_error_string(err));
    throw std::runtime_error(errtxt);
  }
}
