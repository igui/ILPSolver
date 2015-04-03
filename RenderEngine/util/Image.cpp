/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "Image.h"
#include <QFileInfo>
#include <QFile>
#include <QString>
#include <QImage>


Image::Image(const QString & imageCompletePath)
{   
    QFile imageFile (imageCompletePath);

    if(!imageFile.exists())
    {
        QString string = QString("The file %1 does not exist.").arg(imageCompletePath);
		throw std::exception(string.toLatin1().constData());
    }

    if(!imageFile.open(QIODevice::ReadOnly))
    {
		QString string = QString("An error occurred trying to open the image %1.").arg(imageFile.fileName());
        throw std::exception(string.toLatin1().constData());
	}
    QFileInfo fileInfo(imageFile);
    
    QImage image(imageCompletePath);
    if(image.isNull())
    {
		QString error = QString("Unable to load image %1 using Qt.").arg(imageCompletePath);
        throw std::exception(error.toLatin1().constData());
	}

	// image is mirrored vertically for Blender export compatibility
	m_image = image.convertToFormat(QImage::Format_RGBA8888).mirrored(false, true);
    if(m_image.isNull())
    {
        QString string = QString("Image is null.");
        throw std::exception(string.toLatin1().constData());
    }
}

Image::~Image(void)
{
}


unsigned int Image::getWidth() const
{
	return m_image.width();
}

unsigned int Image::getHeight() const
{
	return m_image.height();
}

const unsigned char* Image::constData() const
{
	return m_image.constBits();
}
