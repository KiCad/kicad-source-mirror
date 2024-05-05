#ifndef CImage_HPP_INCLUDED
#define CImage_HPP_INCLUDED
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (c) 2018-2021 3Dconnexion.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file CImage.hpp
 * @brief SiImage_t wrapper.
 */

// stdlib
#include <cstdint>
#include <string>
#include <sstream>

// 3dxware
#include <siappcmd_types.h>

#if defined(_MSC_VER) && _MSC_VER<1800
// #pragma warning(1 : 4519) // convert error C4519 'default template arguments are only allowed on a class template' to warning
#pragma warning(disable : 4519) // disable error C4519
#if _MSC_VER < 1700
#pragma warning(disable : 4482) // warning C4482: nonstandard extension used
#endif
#endif

/// <summary>
/// Determines whether a value is an integer identifier for a resource.
/// </summary>
/// <param name="id">
/// The pointer to be tested whether it contains an integer resource identifier.
/// </param>
/// <returns>
/// true when  id is an integer identifier for a resource, otherwise false when
/// it is typically a pointer to a string </returns>
__inline bool IsIntResource(const char *id) {
#ifdef _WIN32
  return ((reinterpret_cast<uintptr_t>(id)) >> 16) == 0;
#else
  return false;
#endif
}

namespace TDx {
/// <summary>
/// A class that represents the SiImage_t structure.
/// </summary>
class CImage {
public:
  /// <summary>
  /// Initializes a new instance of the <see cref="CImage"/> class.
  /// </summary>
  CImage() : m_type(SiImageType_t::e_none), m_index(0) {
  }

  /// <summary>
  /// Initializes a new instance of the <see cref="CImage"/> class from a <see cref="SiImage_t"/>.
  /// </summary>
  /// <param name="image"></param>
  explicit CImage(const SiImage_t &siImage) : m_id(siImage.id), m_type(siImage.type) {
    if (siImage.size != sizeof(SiImage_t)) {
      throw std::invalid_argument("Invalid SiImage_t structure.");
    }

    switch (m_type) {
    case SiImageType_t::e_image_file:
      m_source = siImage.file.file_name;
      m_index = siImage.file.index;
      break;
    case SiImageType_t::e_resource_file:
      m_source = siImage.resource.file_name;
      m_resourceId = siImage.resource.id;
      m_resourceType = siImage.resource.type;
      m_index = siImage.resource.index;
      break;
    case SiImageType_t::e_image:
      m_source.assign(reinterpret_cast<const char *>(siImage.image.data), siImage.image.size);
      m_index = siImage.image.index;
      break;
    case SiImageType_t::e_none:
    default:
      break;
    }
  }

#if USE_DECLSPEC_PROPERTY
  /// <summary>
  /// Gets or sets the image id.
  /// </summary>
  __declspec(property(get = GetId, put = PutId)) std::string Id;

  /// <summary>
  /// Gets or sets the resource id.
  /// </summary>
  __declspec(property(get = GetResourceId, put = PutResourceId)) std::string ResourceId;

  /// <summary>
  /// Gets the image type.
  /// </summary>
  __declspec(property(get = GetType)) SiImageType_t Type;
#endif

  /// <summary>
  /// Sets the id of the image.
  /// </summary>
  /// <param name="id">A <see cref="std::string"/> representing the name or id of the image.</param>
  void PutId(std::string id) {
    m_id = std::move(id);
  }

  /// <summary>
  /// Gets the image id.
  /// </summary>
  /// <returns>A <see cref="std::string"/> representing the name or id of the image.</returns>
  std::string GetId() const {
    return m_id;
  }

  /// <summary>
  /// Sets the id of the resource in the resource file.
  /// </summary>
  /// <param name="id">A <see cref="std::string"/> representing the name or id of the image in the
  /// resource file.</param>
  /// <remarks>For example for Microsoft resource libraries MAKEINTRESOURCE(216) = "#216".</remarks>
  void PutResourceId(std::string id) {
    m_resourceId = std::move(id);
  }

  /// <summary>
  /// Gets the resource id.
  /// </summary>
  /// <returns>A <see cref="std::string"/> representing the name or id of the image.</returns>
  /// <remarks>For example for Microsoft resource libraries MAKEINTRESOURCE(216) = "#216".</remarks>
  std::string GetResourceId() const {
    return m_resourceId;
  }

  /// <summary>
  /// Gets the image type.
  /// </summary>
  /// <returns>One of <see cref="SiImageType_t"/>.</returns>
  SiImageType_t GetType() const {
    return m_type;
  }


  /// <summary>
  /// Assigns image data to the <see cref="CImage"/> instance.
  /// </summary>
  /// <param name="buffer">The image data. The image may be in any format that can be loaded by
  /// Gdiplus::Bitmap::FromStream() or is in a recognizable svg format.</param>
  /// <returns>true if successful, otherwise false.</returns>
  /// <remarks>The <see cref="CImage"/> type is set to <see
  /// cref="SiImageType_t::e_image"/>.</remarks>
  bool AssignImage(std::string buffer, uint32_t index = 0) {
    m_source = std::move(buffer);
    m_type = e_image;
    m_index = index;
    m_resourceId.clear();
    m_resourceType.clear();
    return true;
  }

  /// <summary>
  /// Initializes a new instance of the <see cref="CImage"/> class that contains the data for an
  /// image held in a resource file.
  /// </summary>
  /// <param name="resourceFile">The name of the resource file.</param>
  /// <param name="resourceId">The name or id of the image in the resource file. i.e.
  /// MAKEINTRESOURCE(216) = "#216".</param>
  /// <param name="resourceType">The type of the resource in the resource file. i.e. RT_BITMAP =
  /// "#2".</param>
  /// <param name="index">The index in a multi-image resource.</param>
  /// <param name="id">The id of the command to associate with this image.</param>
  /// <returns>A <see cref="CImage"/> instance.</returns>
  template <class T = CImage>
  static T FromResource(const std::string &resourceFile, const char *resourceId,
                        const char *resourceType, uint32_t index = 0, const char *id = nullptr) {

    std::string r_id;
    if (resourceId != nullptr) {
      if (IsIntResource(resourceId)) {
        std::ostringstream stream;
        stream << "#" << reinterpret_cast<uintptr_t>(resourceId);
        r_id = stream.str();
      } else {
        r_id = resourceId;
      }
    }

    std::string r_type;
    if (resourceType != nullptr) {
      if (IsIntResource(resourceType)) {
        std::ostringstream stream;
        stream << "#" << reinterpret_cast<uintptr_t>(resourceType);
        r_type = stream.str();
      } else {
        r_type = resourceType;
      }
    }

    T image(resourceFile, r_id, r_type, id != nullptr ? id : "", e_resource_file,
            index);
    return image;
  }

  /// <summary>
  /// Initializes a new instance of the <see cref="CImage"/> class that contains the data for an
  /// image in a file.
  /// </summary>
  /// <param name="filename">The name of the image file.</param>
  /// <param name="index">The index in a multi-image file.</param>
  /// <param name="id">The id of the command to associate with this image.</param>
  /// <returns>A <see cref="CImage"/> instance.</returns>
  template <class T = CImage>
  static T FromFile(const std::string &filename, uint32_t index = 0, const char *id = nullptr) {
    T image(filename, id != nullptr ? id : "", e_image_file, index);
    return image;
  }

  /// <summary>
  /// Initializes a new instance of the CImage class that contains the data for an image in a file.
  /// </summary>
  /// <param name="buffer">The image data. The image may be in any format that can be loaded by
  /// Gdiplus::Bitmap::FromStream() or is in a recognizable svg format.</param>
  /// <param name="index">The index in a multi-image file.</param>
  /// <param name="id">The id of the command to associate with this image.</param>
  /// <returns>A <see cref="CImage"/> instance.</returns>
  template <class T = CImage>
  static T FromData(const std::string &buffer, uint32_t index = 0, const char *id = nullptr) {
    T image(buffer, id != nullptr ? id : "", e_image, index);
    return image;
  }

  /// <summary>
  /// Returns an <see cref="SiImage_t"/> view of the <see cref="CImage"/> instance.
  /// </summary>
  /// <returns>A <see cref="SiImage_t"/>.</returns>
  operator SiImage_t() const {
    SiImage_t siImage = {sizeof(SiImage_t), m_type, m_id.c_str(), {{nullptr, nullptr, nullptr, 0}}};
    switch (m_type) {
    case SiImageType_t::e_image_file:
      siImage.file.file_name = m_source.c_str();
      siImage.file.index = m_index;
      break;
    case SiImageType_t::e_resource_file:
      siImage.resource.file_name = m_source.c_str();
      siImage.resource.id = m_resourceId.c_str();
      siImage.resource.type = m_resourceType.c_str();
      siImage.resource.index = m_index;
      break;
    case SiImageType_t::e_image:
      siImage.image.data = reinterpret_cast<const uint8_t *>(m_source.data());
      siImage.image.size = m_source.size();
      siImage.image.index = m_index;
      break;
    case SiImageType_t::e_none:
      break;
    default:
      siImage.type = SiImageType_t::e_none;
      break;
    }

    return siImage;
  }

  /// <summary>
  /// checks whether the image is empty.
  /// </summary>
  /// <returns>true if the image contains data, otherwise false.</returns>
  bool empty() const {
    return m_type == SiImageType_t::e_none || (m_type == SiImageType_t::e_image && m_source.empty());
  }

protected:

  CImage(std::string source, std::string id, SiImageType_t type, uint32_t index = 0)
      : m_source(std::move(source)), m_id(std::move(id)), m_type(type), m_index(index) {
  }

  CImage(std::string source, std::string resourceName, std::string resourceType, std::string id,
         SiImageType_t type, uint32_t index = 0)
      : m_source(std::move(source)), m_resourceId(std::move(resourceName)),
        m_resourceType(std::move(resourceType)), m_id(std::move(id)), m_type(type), m_index(index) {
  }

protected:
  std::string m_source;
  std::string m_resourceId;
  std::string m_resourceType;
  std::string m_id;
  SiImageType_t m_type;
  uint32_t m_index;
};
} // namespace TDx
#endif // CImage_HPP_INCLUDED
