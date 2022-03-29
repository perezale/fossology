<?php
/***********************************************************
 Copyright (C) 2009-2013 Hewlett-Packard Development Company, L.P.
 Copyright (C) 2015, Siemens AG

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ***********************************************************/

use Fossology\Lib\Plugin\AgentPlugin;

/**
 * @dir
 * @brief UI plugin of scanoss Agent
 * @file
 * @brief UI plugin of  scanoss Agent
 * @class ScanossAgentPlugin
 * @brief UI plugin of  scanoss Agent
 */
class ScanossAgentPlugin extends AgentPlugin
{
  public function __construct() {
    $this->Name = "agent_scanoss";
    $this->Title = _("SCANOSS Toolkit");
    $this->AgentName = "scanoss";

    parent::__construct();
  }

/**
   * @brief Render HTML from template
   * @param array $vars Variables using in template
   * @return string HTML rendered from agent_decider.html.twig template
   */
  public function renderContent(&$vars)
  {
    $renderer = $GLOBALS['container']->get('twig.environment');
    return $renderer->load('scanoss.html.twig')->render($vars);
  }



  /**
   * @copydoc Fossology::Lib::Plugin::AgentPlugin::AgentHasResults()
   * @see Fossology::Lib::Plugin::AgentPlugin::AgentHasResults()
   */
  function AgentHasResults($uploadId=0)
  {
    return CheckARS($uploadId, $this->AgentName, "SCANOSS Snippet and License scan", "scanoss_ars");
  }

  /**
   * @copydoc Fossology::Lib::Plugin::AgentPlugin::preInstall()
   * @see Fossology::Lib::Plugin::AgentPlugin::preInstall()
   */
  function preInstall()
  {
    $dbManager = $GLOBALS['container']->get('db.manager');
    $latestPkgAgent = $dbManager->getSingleRow("SELECT agent_enabled FROM agent WHERE agent_name=$1 ORDER BY agent_ts LIMIT 1",array('scanoss'));
    if (isset($latestPkgAgent) && !$dbManager->booleanFromDb($latestPkgAgent['agent_enabled']))
    {
      return 0;
    }
    menu_insert("Agents::" . $this->Title, 0, $this->Name);
  }

}

register_plugin(new ScanossAgentPlugin());