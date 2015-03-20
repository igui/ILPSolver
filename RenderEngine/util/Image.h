/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/


#pragma once
class QString;
class QFile;

#include <QImage>

// An RGBA formatted image 
class Image
{
public:
    Image(const QString & imageCompletePath);
    ~Image(void);
    unsigned int getWidth() const;
    unsigned int getHeight() const;
    const unsigned char* constData() const;

private:
	QImage m_image;
};