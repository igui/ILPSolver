/* 
 * Copyright (c) 2013 Opposite Renderer
 * For the full copyright and license information, please view the LICENSE.txt
 * file that was distributed with this source code.
*/

#include "SceneFactory.h"
#include "scene/Scene.h"


SceneFactory::SceneFactory(void)
{
}


SceneFactory::~SceneFactory(void)
{
}

IScene* SceneFactory::getSceneByName( const char* name )
{
	return Scene::createFromFile(name);
}
