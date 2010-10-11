<?php
// $Id$
/**
 * @file * Admin include file. 
*/

/**
 * Presets overview page.
 */
function presets_overview() {
  $item = menu_get_item('admin/presets');
  $content = system_admin_menu_block($item);

  $output = theme('admin_block_content', $content);

  return $output;
}

/** 
 * Displays the form for the standard settings tab.
 * 
 * @return array 
 * A structured array for use with Forms API. 
*/ 
function presets_page($preset) {
  $output = '';
  include_once './includes/install.inc';
  $presets = presets_get_presets();
  // Get current list of modules.
  $files = module_rebuild_cache();
  $dependencies = array();
  $download = array();
  $enable = array();
  $config = array();
  foreach($presets[$preset]['modules'] as $module => $info) {
    if (!$files[$module]) {
      $download[$module] = $info;
    }
    else {
      if (!empty($info['submodules'])) {
        foreach($info['submodules'] as $submodule) {
          if ($files[$submodule]->status == 0) {
            $enable[$submodule] = $files[$submodule]->info;
            foreach($files[$submodule]->info['dependencies'] as $dependency) {
              $dependencies[] = $dependency;
            }
          }
        }
      }
      if ($files[$module]->status == 0) {
        $enable[$module] = $info;
        foreach($files[$module]->info['dependencies'] as $dependency) {
          $dependencies[] = $dependency;
        }
      }
      elseif ($files[$module]->status == 1) {
        $config[$module] = $info;
      }
    }
  }

  foreach($dependencies as $dependency) {
    if (!$files[$dependency]) {
      $download[] = $dependency;
    }
  }

  if (!empty($download)) {
    $settings = array();
    $settings['title'] = array(
      'title' => 'Download Modules',
      'value' => 'Module',
      'action' => 'Action',
      'severity' => -1,
    );
    foreach($download as $module => $info) {
      $settings[$module] = array(
        'title' => $module,
        'value' => $info['name'],
        'action' => l('Download', 'http://drupal.org/project/' . $module, array('attributes' => array('target' => '_blank'))),
        'severity' => 2,
      );
    }
    $output .= theme('presets_report', $settings);
  }
  else {
    $settings = array();
    $settings['title'] = array(
      'title' => 'Download Modules',
      'value' => 'Module',
      'description' => 'All recommended modules have been downloaded',
      'severity' => 0,
    );
    $output .= theme('presets_report', $settings);
  }

  $form['enable'] = array(
    '#type' => 'fieldset',
    '#title' => 'Enable these modules'
  );

  if (!empty($enable)) {
    $settings = array();
    $settings['title'] = array(
      'title' => 'Enable Modules',
      'value' => 'Module',
      'action' => 'Action',
      'severity' => -1,
    );
    foreach($enable as $module => $info) {
      $settings[$module] = array(
        'title' => $module,
        'value' => $info['name'],
        'action' => l('Enable', 'admin/build/modules'),
        'severity' => 2,
      );
    }
    $output .= theme('presets_report', $settings);
  }
  else {
    $settings = array();
    $settings['title'] = array(
      'title' => 'Enable Modules',
      'value' => 'Module',
      'description' => 'All recommended modules have been enabled',
      'severity' => 0,
    );
    $output .= theme('presets_report', $settings);
  }

  foreach($config as $module => $info) {
    if ($info['settings']) {
      if ($presets[$preset]['file']) {
        include_once(drupal_get_path('module', $preset) . '/' . $presets[$preset]['file']);
      }
      $settings = array();
      $settings['title'] = array(
        'title' => $info['name'],
        'value' => 'Setting',
        'action' => 'Action',
        'severity' => -1,
      );
      foreach($info['settings'] as $setting) {
        $function = $preset . '_' . $module . '_' . $setting;
        if (function_exists($function)) {
          $settings[$setting] = $function();
          if ($settings[$setting]['severity'] != 0 && function_exists($function . '_reset')) {
            $settings[$setting]['action'] = l('Reset', 'admin/presets/settings/' . $preset . '/' . $module . '/' . $setting);
          }
          $output .= theme('presets_report', $settings);

        }
      }
    }
  }

  return $output;
  //Add the system buttons to the form
//  $form = system_settings_form($form);
//  return $form;
}

function presets_disabled_modules() {
  $modules = array();
  $presets = presets_get_presets();
  $files = module_rebuild_cache();
  foreach($presets as $preset => $preset_info){
    foreach($preset_info['modules'] as $module => $info) {
      if ($files[$module] && $files[$module]->status == 0) {
        $dependencies_met = TRUE;
        foreach($files[$module]->info['dependencies'] as $dependency) {
          if (!$files[$dependency]) {
            $dependencies_met = FALSE;
          }
        }
        if ($dependencies_met) {
          $modules[$module] = $files[$module]->info['name'];
        }
      }
      if (!empty($info['submodules'])) {
        foreach($info['submodules'] as $submodule) {
          if ($files[$submodule] && $files[$submodule]->status == 0) {
            $dependencies_met = TRUE;
            foreach($files[$submodule]->info['dependencies'] as $dependency) {
              if (!$files[$dependency]) {
                $dependencies_met = FALSE;
              }
            }
            if ($dependencies_met) {
              $modules[$submodule] = $files[$submodule]->info['name'];
            }
          }
        }
      }
    }
  }
  return $modules;
}

/**
 * Generic preset reset page callback.
 */
function presets_reset_callback($preset, $module, $setting) {
  $presets = presets_get_presets();
  if ($presets[$preset]['file']) {
    include_once(drupal_get_path('module', $preset) . '/' . $presets[$preset]['file']);
  }
  $function = $preset . '_' . $module . '_' . $setting . '_reset';
  if (function_exists($function)) {
    $function();
  }
  else {
    drupal_set_message('Function: ' . $function  . '() not found');
  }
  drupal_goto('admin/presets/' . $preset);
}

/**
 * Theme settings presets report.
 *
 * @param $requirements
 *   An array of requirements.
 * @ingroup themeable
 */
function theme_presets_report($requirements) {
  $i = 0;
  $output = '<table class="system-status-report">';
  foreach ($requirements as $requirement) {
    if (empty($requirement['#type'])) {
      $class = ++$i % 2 == 0 ? 'even' : 'odd';

      $classes = array(
        REQUIREMENT_INFO => 'info',
        REQUIREMENT_OK => 'ok',
        REQUIREMENT_WARNING => 'warning',
        REQUIREMENT_ERROR => 'error',
      );
      $class = $classes[isset($requirement['severity']) ? (int)$requirement['severity'] : 0] .' '. $class;

      // Output table row(s)
      if (!empty($requirement['description'])) {
        $output .= '<tr class="'. $class .' merge-down"><th>'. $requirement['title'] .'</th><td>'. $requirement['value'] .'</td><td>'. $requirement['action'] .'</td></tr>';
        $output .= '<tr class="'. $class .' merge-up"><td colspan="3">'. $requirement['description'] .'</td></tr>';
      }
      else {
        $output .= '<tr class="'. $class .'"><th>'. $requirement['title'] .'</th><td>'. $requirement['value'] .'</td><td>'. $requirement['action'] .'</td></tr>';
      }
    }
  }

  $output .= '</table>';
  return $output;
}

