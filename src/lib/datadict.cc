/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * datadict.cc
 */

#include "dicom.h"

namespace dicom {

typedef struct _element_registry_struct_ {
  const char *tag_str;
  const char *name;
  const char *keyword;
  vr_t vr;
  const char *vr_str;
  const char *vm;
  const char *retired;
} ElementRegistry;

#include "datadict.inc.cxx"
/*
 const char* DATADICTIONARY_VERSION = "DICOM PS3.6 2020c";
 const int SIZE_ELEMENT_REGISTRY = 4206;
 const int SIZE_INDEX_TAGS_WITH_XX = 88;
 const int SIZE_INDEX_KEYWORD = 4202;

 static const ElementRegistry element_registry[] = {}
 static const tag_t tags_registry[] = {}
 static const int tags_xx_index[] = {}
 static const int keyword_index[] = {}
 */

// -----------------------------------------------------------------------------
static uint32_t fnv1a(const char *ptr, size_t size) {
  uint32_t hash;

  hash = 0x811c9dc5;  // 2166136261, 32-bit FNV offset
  while (size--) {
    hash ^= uint32_t(*ptr++);
    hash *= 0x1000193;  // 16777619, 32-bit FNV prive
  }
  return hash;
}

tsuid_t UID::from_uidvalue(const char *uidvalue) {
  int size = strlen(uidvalue);

  uint32_t hash = fnv1a(uidvalue, size) & 0xffff;  // use lower 16 bit

  // search from tsuid_hash_index

  int imin, imax;
  imin = 0;
  imax = sizeof(tsuid_hash_index) / 2 / sizeof(int) - 1;

  while (imin < imax) {
    int imid = (imin + imax) / 2;

    if (tsuid_hash_index[imid * 2] < hash)
      imin = imid + 1;
    else
      imax = imid;
  }

  tsuid_t u = (tsuid_t)tsuid_hash_index[imin * 2 + 1];
  if ((imax == imin) && (tsuid_hash_index[imin * 2] == hash) &&
      strncmp(uidvalue, uid_registry[u * 3], size) == 0) {
    return u;
  }

  return UID::UNKNOWN;
}

const char *UID::to_uidname(tsuid_t uid) {
  if (uid >= 0 && uid < sizeof(uid_registry) / sizeof(const char *))
    return uid_registry[uid * 3 + 1];
  else
    return "";
}

const char *UID::to_uidvalue(tsuid_t uid) {
  if (uid >= 0 && uid < sizeof(uid_registry) / sizeof(const char *))
    return uid_registry[uid * 3];
  else
    return "";
}

// get UID name from UID value
const char *UID::uidvalue_to_uidname(const char *uidvalue) {
  int size = strlen(uidvalue);

  uint32_t hash = fnv1a(uidvalue, size) & 0xffff;  // use lower 16 bit

  int imin, imax;
  tsuid_t u;

  // search from tsuid_hash_index
  imin = 0;
  imax = sizeof(tsuid_hash_index) / 2 / sizeof(int) - 1;

  while (imin < imax) {
    int imid = (imin + imax) / 2;

    if (tsuid_hash_index[imid * 2] < hash)
      imin = imid + 1;
    else
      imax = imid;
  }

  u = (tsuid_t)tsuid_hash_index[imin * 2 + 1];
  if ((imax == imin) && (tsuid_hash_index[imin * 2] == hash) &&
      strncmp(uidvalue, uid_registry[u * 3], size) == 0) {
    return uid_registry[u * 3 + 1];
  }

  // search from uid_hash_index
  imin = 0;
  imax = sizeof(uid_hash_index) / 2 / sizeof(int) - 1;

  while (imin < imax) {
    int imid = (imin + imax) / 2;

    if (uid_hash_index[imid * 2] < hash)
      imin = imid + 1;
    else
      imax = imid;
  }

  u = (tsuid_t)uid_hash_index[imin * 2 + 1];
  if ((imax == imin) && (uid_hash_index[imin * 2] == hash) &&
      strncmp(uidvalue, uid_registry[u * 3], size) == 0) {
    return uid_registry[u * 3 + 1];
  }

  return "(Unknown UID)";
}

// -----------------------------------------------------------------------------

static int _compare_tag_xx(const char *a, char *b) {
  while (*a) {
    if (*a == 'x' || *a == *b) {
      a++;
      b++;
    } else {
      if (*a > *b) return 1;
      if (*a < *b) return -1;
    }
  }
  return 0;
}

// Search a Tag from Element Registry
static ElementRegistry *_find_tag(tag_t key) {
  int imin, imax;

  // Try find key from 'tag_t tags_registry[]'

  imin = 0;
  imax = SIZE_ELEMENT_REGISTRY - 1;
  while (imin < imax) {
    int imid = (imin + imax) / 2;

    if (tags_registry[imid] < key)
      imin = imid + 1;
    else
      imax = imid;
  }

  if ((imax == imin) && (tags_registry[imin] == key))
    return (ElementRegistry *)element_registry + imin;

  // Try find key from 'int tags_xx_index[]'

  imin = 0;
  imax = SIZE_INDEX_TAGS_WITH_XX - 1;
  char key_str[16];
  sprintf(key_str, "(%04x,%04x)", TAG::group(key), TAG::element(key));

  while (imin < imax) {
    int imid = (imin + imax) / 2;

    if (_compare_tag_xx(element_registry[tags_xx_index[imid]].tag_str,
                        key_str) < 0)
      imin = imid + 1;
    else
      imax = imid;
  }

  if ((_compare_tag_xx(element_registry[tags_xx_index[imin]].tag_str,
                       key_str) == 0) &&
      (imax == imin))
    return (ElementRegistry *)element_registry + tags_xx_index[imin];

  // can't find tag from registry

  return nullptr;
}

// Tag -> Element's VR
vr_t TAG::get_vr(tag_t tag) {
  if (TAG::element(tag) == 0)  // group length
    return VR::UL;

  ElementRegistry *result = _find_tag(tag);
  if (result) return result->vr;

  return VR::NONE;
}

// Tag -> Elements' Name
const char *TAG::name(tag_t tag) {
  ElementRegistry *result = _find_tag(tag);
  if (result) return result->name;

  if (TAG::element(tag) == 0) return "Group Length";
  if (TAG::group(tag) & 1) {
    if (TAG::element(tag) >= 0x10 && TAG::element(tag) <= 0xff)
      return "Private Creator Data Element";
    else
      return "Private Data Element";
  }
  return "Unknown Data Element";
}

// Tag -> Elements' Keyword
const char *TAG::keyword(tag_t tag) {
  ElementRegistry *result = _find_tag(tag);
  if (result) return result->keyword;
  if (TAG::element(tag) == 0) return "(Group Length)";
  if (TAG::group(tag) & 1) {
    if (TAG::element(tag) >= 0x10 && TAG::element(tag) <= 0xff)
      return "(Private Creator Data Element)";
    else
      return "(Private Data Elements)";
  }
  return "(Unknown Data Elements)";
}

// Element's Keyword -> Tag
tag_t TAG::from_keyword(const char *keyword) {
  int imin = 0, imax = SIZE_INDEX_KEYWORD - 1;
  while (imin < imax) {
    int imid = (imin + imax) / 2;
    if (strcmp(element_registry[keyword_index[imid]].keyword, keyword) < 0)
      imin = imid + 1;
    else
      imax = imid;
  }

  if ((strcmp(element_registry[keyword_index[imin]].keyword, keyword) == 0) &&
      (imax == imin)) {
    return tags_registry[keyword_index[imin]];
  } else
    return 0xFFFFFFFF;
}

// -----------------------------------------------------------------------------

const char* VR::repr(vr_t vr) {
  switch (vr) {
    case VR::NONE: return "NONE"; break;

// UID const names are generated by 'codegenerator_valueprepresentations.py'.
// Place $$Generated code in 'src/lib/datadict.cc'.
    case VR::AE: return "AE"; break;
    case VR::AS: return "AS"; break;
    case VR::AT: return "AT"; break;
    case VR::CS: return "CS"; break;
    case VR::DA: return "DA"; break;
    case VR::DS: return "DS"; break;
    case VR::DT: return "DT"; break;
    case VR::FL: return "FL"; break;
    case VR::FD: return "FD"; break;
    case VR::IS: return "IS"; break;
    case VR::LO: return "LO"; break;
    case VR::LT: return "LT"; break;
    case VR::OB: return "OB"; break;
    case VR::OD: return "OD"; break;
    case VR::OF: return "OF"; break;
    case VR::OL: return "OL"; break;
    case VR::OV: return "OV"; break;
    case VR::OW: return "OW"; break;
    case VR::PN: return "PN"; break;
    case VR::SH: return "SH"; break;
    case VR::SL: return "SL"; break;
    case VR::SQ: return "SQ"; break;
    case VR::SS: return "SS"; break;
    case VR::ST: return "ST"; break;
    case VR::SV: return "SV"; break;
    case VR::TM: return "TM"; break;
    case VR::UC: return "UC"; break;
    case VR::UI: return "UI"; break;
    case VR::UL: return "UL"; break;
    case VR::UN: return "UN"; break;
    case VR::UR: return "UR"; break;
    case VR::US: return "US"; break;
    case VR::UT: return "UT"; break;
    case VR::UV: return "UV"; break;
// $$End of generated code.

    case VR::PIXSEQ: return "PIXSEQ"; break;
    case VR::OFFSET: return "OFFSET"; break;
    default:
      break;
  }
  return "UNKNOWN";
}


vr_t VR::from_string(const char *s) {
  if (!s || strlen(s) < 2)
    return VR::NONE;

  return from_uint16le(s[0] + s[1]*256);
}

vr_t VR::from_uint16le(uint16_t u)
{
  vr_t vr;
  switch (u) {
// codelet to convert uint16le to vr_t are generated by 'codegenerator_valueprepresentations.py'.
// Place $$Generated code in 'src/lib/datadict.cc'.
    case 0x4541: vr = VR::AE; break;
    case 0x5341: vr = VR::AS; break;
    case 0x5441: vr = VR::AT; break;
    case 0x5343: vr = VR::CS; break;
    case 0x4144: vr = VR::DA; break;
    case 0x5344: vr = VR::DS; break;
    case 0x5444: vr = VR::DT; break;
    case 0x4c46: vr = VR::FL; break;
    case 0x4446: vr = VR::FD; break;
    case 0x5349: vr = VR::IS; break;
    case 0x4f4c: vr = VR::LO; break;
    case 0x544c: vr = VR::LT; break;
    case 0x424f: vr = VR::OB; break;
    case 0x444f: vr = VR::OD; break;
    case 0x464f: vr = VR::OF; break;
    case 0x4c4f: vr = VR::OL; break;
    case 0x564f: vr = VR::OV; break;
    case 0x574f: vr = VR::OW; break;
    case 0x4e50: vr = VR::PN; break;
    case 0x4853: vr = VR::SH; break;
    case 0x4c53: vr = VR::SL; break;
    case 0x5153: vr = VR::SQ; break;
    case 0x5353: vr = VR::SS; break;
    case 0x5453: vr = VR::ST; break;
    case 0x5653: vr = VR::SV; break;
    case 0x4d54: vr = VR::TM; break;
    case 0x4355: vr = VR::UC; break;
    case 0x4955: vr = VR::UI; break;
    case 0x4c55: vr = VR::UL; break;
    case 0x4e55: vr = VR::UN; break;
    case 0x5255: vr = VR::UR; break;
    case 0x5355: vr = VR::US; break;
    case 0x5455: vr = VR::UT; break;
    case 0x5655: vr = VR::UV; break;
// $$End of generated code.
    default:
      vr = VR::NONE;
      break;
  }
  return vr;
}

std::string TAG::repr(tag_t tag) {
  static char buf[16];
  sprintf(buf, "(%04X,%04X)", TAG::group(tag), TAG::element(tag));
  return std::string(buf);
}

}  // namespace dicom
