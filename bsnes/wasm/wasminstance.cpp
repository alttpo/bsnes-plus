
WASMMessage::WASMMessage(const uint8_t * const data, uint16_t size) : m_data(new uint8_t[size]), m_size(size) {
  memcpy((void *)m_data, (const void *)data, size);
}

WASMMessage::~WASMMessage() {
  delete[] m_data;
}

// ZipArchive:
//////////

ZipArchive::ZipArchive(const uint8_t *data, size_t size) {
  mz_zip_reader_init_mem(&m_zar, (const void *) data, size, 0);
}

ZipArchive::~ZipArchive() {
  mz_zip_reader_end(&m_zar);
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
    return ZipArchive::FileHandle(-1);
  }

  return ZipArchive::FileHandle(h);
}

bool ZipArchive::file_size(ZipArchive::FileHandle fh, uint64_t *o_size) {
  mz_zip_archive_file_stat s;
  if (!mz_zip_reader_file_stat(&m_zar, fh, &s)) {
    return false;
  }

  *o_size = s.m_uncomp_size;
  return true;
}

bool ZipArchive::file_extract(ZipArchive::FileHandle fh, void *o_data, size_t i_size) {
  return mz_zip_reader_extract_to_mem(&m_zar, fh, o_data, i_size, 0) == 0;
}

// Base:
//////////

WASMInstanceBase::WASMInstanceBase(WASMInterface* interface, const std::string &key, const std::shared_ptr<ZipArchive> &za)
  : m_interface(interface), m_key(key), m_za(za), m_data(nullptr), m_size(0)
{
  auto fh = m_za->file_locate("main.wasm");
  if (!fh) {
    throw std::runtime_error("missing required main.wasm in zip archive");
  }

  if (!m_za->file_size(fh, &m_size)) {
    throw std::runtime_error("failed to retrieve file size of main.wasm in zip archive");
  }

  // extract main.wasm into m_data/m_size:
  m_data = new uint8_t[m_size];
  if (!m_za->file_extract(fh, m_data, m_size)) {
    throw std::runtime_error("failed to extract main.wasm in zip archive");
  }
}

WASMInstanceBase::~WASMInstanceBase() {
  delete[] m_data;
}

void WASMInstanceBase::msg_enqueue(const std::shared_ptr<WASMMessage>& msg) {
  //printf("msg_enqueue(%p, %u)\n", msg->m_data, msg->m_size);
  m_msgs.push(msg);

  invoke("on_msg_recv", nullptr);
}

std::shared_ptr<WASMMessage> WASMInstanceBase::msg_dequeue() {
  auto msg = m_msgs.front();
  //printf("msg_dequeue() -> {%p, %u}\n", msg->m_data, msg->m_size);
  m_msgs.pop();
  return msg;
}

bool WASMInstanceBase::msg_size(uint16_t *o_size) {
  if (m_msgs.empty()) {
    //printf("msg_size() -> false\n");
    return false;
  }

  *o_size = m_msgs.front()->m_size;
  //printf("msg_size() -> true, %u\n", *o_size);
  return true;
}

#ifdef WASM_USE_M3
#  include "wasminstance_m3.cpp"
#else

#endif
