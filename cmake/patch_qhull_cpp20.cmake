# Patch Qhull 8.0.2 libqhullcpp headers for C++20 (GCC 11+ rejects template-ids on ctors/dtors).
if(NOT QHULL_SOURCE_DIR)
  message(FATAL_ERROR "patch_qhull_cpp20.cmake requires QHULL_SOURCE_DIR")
endif()

set(_linked_list "${QHULL_SOURCE_DIR}/src/libqhullcpp/QhullLinkedList.h")
set(_set_h "${QHULL_SOURCE_DIR}/src/libqhullcpp/QhullSet.h")

foreach(_file IN ITEMS "${_linked_list}" "${_set_h}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "Qhull header missing: ${_file}")
  endif()
endforeach()

file(READ "${_linked_list}" _linked_list_content)
if(_linked_list_content MATCHES "QhullLinkedList<T>\\(T b")
  string(REPLACE "QhullLinkedList<T>(T b, T e)" "QhullLinkedList(T b, T e)" _linked_list_content
         "${_linked_list_content}")
  string(REPLACE "QhullLinkedList<T>(const QhullLinkedList<T> &other)"
         "QhullLinkedList(const QhullLinkedList<T> &other)" _linked_list_content "${_linked_list_content}")
  string(REPLACE "QhullLinkedList<T> &       operator=(const QhullLinkedList<T> &other)"
         "QhullLinkedList &       operator=(const QhullLinkedList<T> &other)" _linked_list_content
         "${_linked_list_content}")
  string(REPLACE "~QhullLinkedList<T>()" "~QhullLinkedList()" _linked_list_content "${_linked_list_content}")
  string(REPLACE "QhullLinkedList<T>() {}" "QhullLinkedList() {}" _linked_list_content "${_linked_list_content}")
  file(WRITE "${_linked_list}" "${_linked_list_content}")
  message(STATUS "Patched QhullLinkedList.h for C++20")
endif()

file(READ "${_set_h}" _set_content)
if(_set_content MATCHES "QhullSet<T>\\(const Qhull")
  string(REPLACE "QhullSet<T>(const Qhull &q, setT *s)" "QhullSet(const Qhull &q, setT *s)" _set_content
         "${_set_content}")
  string(REPLACE "QhullSet<T>(QhullQh *qqh, setT *s)" "QhullSet(QhullQh *qqh, setT *s)" _set_content
         "${_set_content}")
  string(REPLACE "QhullSet<T>(const QhullSet<T> &other)" "QhullSet(const QhullSet<T> &other)" _set_content
         "${_set_content}")
  string(REPLACE "QhullSet<T> &       operator=(const QhullSet<T> &other)"
         "QhullSet &       operator=(const QhullSet<T> &other)" _set_content "${_set_content}")
  string(REPLACE "~QhullSet<T>()" "~QhullSet()" _set_content "${_set_content}")
  string(REPLACE "QhullSet<T>();" "QhullSet();" _set_content "${_set_content}")
  string(REPLACE "QhullSetIterator<T>(const QhullSet<T> &s)" "QhullSetIterator(const QhullSet<T> &s)"
         _set_content "${_set_content}")
  string(REPLACE "QhullSetIterator<T>(const QhullSetIterator<T> &o)"
         "QhullSetIterator(const QhullSetIterator<T> &o)" _set_content "${_set_content}")
  string(REPLACE "QhullSetIterator<T> &operator=(const QhullSetIterator<T> &o)"
         "QhullSetIterator &operator=(const QhullSetIterator<T> &o)" _set_content "${_set_content}")
  file(WRITE "${_set_h}" "${_set_content}")
  message(STATUS "Patched QhullSet.h for C++20")
endif()
