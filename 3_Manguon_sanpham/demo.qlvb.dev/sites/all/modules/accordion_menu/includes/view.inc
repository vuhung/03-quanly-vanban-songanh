<?php
/**
 * @file
 * Provides view routines.
 *
 * @author Jim Berry ("solotandem", http://drupal.org/user/240748)
 */

/**
 * Implements hook_block_view().
 */
function _accordion_menu_block_view($delta) {
  $config = accordion_menu_config($delta);
  $config['menu_name'] = strtolower(str_replace(array('_', ' '), '-', $config['menu_name']));
  $tree = menu_tree_page_data($config['menu_source']);

  // Localize the tree.
  if (module_exists('i18nmenu')) {
    i18nmenu_localize_tree($tree);
  }

  $active_menu = '';
  $content = accordion_menu_output($tree, $config, $active_menu);
  // Set active menu to false and collapsible: true.
  if ($active_menu === '') {
    $active_menu = 'false';
  }

  // For jQuery 1.6 options, see http://jqueryui.com/demos/accordion/.
  foreach (accordion_menu_js_settings($delta, $config) as $key => $value) {
    $parts = explode('_', $key);
    if (count($parts) > 1) {
      // Example: change auto_height to $autoHeight.
      $key = $parts[0] . ucfirst($parts[1]);
    }
    if ($value != 'true' && $value != 'false') {
      $value = "'$value'";
    }
    $options[] = "$key: $value";
  }
  $options[] = "cookie: { path: '/' }"; // @todo Is this a valid option?
  $options[] = "active: " . $active_menu;

  // Other settings tried before settling on this code:
  // - collapsible: false
  // - event: 'click'
  // - navigation: true
  $js = "jQuery(function() {\n";
  $js .= "jQuery('.accordion-menu-" . $delta . "').accordion({ " . implode(', ', $options) . " });\n";
  $js .= "});";

  jquery_ui_add('ui.accordion');
  drupal_add_js($js, 'inline', $config['script_scope']);

  $block['subject'] = '<none>'; // This is later overridden by block['title'].
  $block['content'] = $content;
  if ($block['content']) {
    $hooks = array(
      'accordion_menu_wrapper__' . str_replace('-', '_', $config['delta']),
      'accordion_menu_wrapper__' . str_replace('-', '_', $config['menu_name']),
      'accordion_menu_wrapper',
    );
    $block['content'] = theme($hooks, $block['content'], $config, $config['delta']);
  }
  return $block;
}

/**
 * Returns jQuery UI accordion settings.
 *
 * @todo String input needs to be passed through check_plain().
 *
 * @param string $delta
 *   The delta that uniquely identifies the block in the block system.
 *
 * @return array
 *   An associative array of JS settings with booleans converted to strings.
 */
function accordion_menu_js_settings($delta, $config) {
  $config_nojs = array(
    'delta' => $delta,
    'menu_name' => 'Menu ' . $delta,
    'menu_source' => 'navigation',
    'script_scope' => 'footer',
    'header_link' => FALSE,
  );

  $config = array_diff_key($config, $config_nojs);
  foreach (array('auto_height', 'clear_style', 'collapsible', 'fill_space', 'navigation') as $key) {
    $config[$key] = accordion_menu_boolean_option($config[$key]);
  }
  return $config;
}

/**
 * Returns string representation of boolean option.
 *
 * @param string $boolean
 *   A boolean value.
 *
 * @return string
 *   A string representation of the boolean value.
 */
function accordion_menu_boolean_option($boolean) {
  return $boolean ? 'true' : 'false';
}

/**
 * Returns rendered accordion menu tree.
 *
 * @param array $tree
 *   An associative array of the menu tree.
 * @param string $config
 *   The configuration settings for the menu block.
 * @param string $active_menu
 *   The active menu item for the JS settings.
 *
 * @return string
 *   An HTML string of the menu tree.
 *
 * @see menu_tree_output()
 */
function accordion_menu_output($tree, $config, &$active_menu = '') {
  $output = '';
  $items = array();

  // Pull out just the menu items we are going to render so that we
  // get an accurate count for the first/last classes.
  foreach ($tree as $data) {
    if (!$data['link']['hidden']) {
      $items[] = $data;
    }
  }

  $item_count = count($items);
  $flip = array('even' => 'odd', 'odd' => 'even');
  $zebra = 'even';
  foreach ($items as $i => $data) {
    $subtree = _accordion_menu_subtree($data['link']);

    // Add CSS classes.
    $class = array();
    $class[] = 'accordion-header-' . ($i + 1);
    if ($i == 0) {
      $class[] = 'first';
    }
    if ($i == $item_count - 1) {
      $class[] = 'last';
    }
    if ($subtree) {
      $class[] = 'has-children';
    }
    else {
      $class[] = 'no-children';
    }
    if ($data['link']['in_active_trail']) {
      $class[] = 'active-trail';
      $data['link']['localized_options']['attributes']['class'] .= ' active-trail';
    }
    if ($data['link']['href'] == $_GET['q'] || ($data['link']['href'] == '<front>' && drupal_is_front_page())) {
      $class[] = 'active';
    }
    $class[] = $zebra = $flip[$zebra];
    $class[] = 'menu-mlid-' . $data['link']['mlid'];
    $class = implode(' ', $class);

    // Allow menu-specific theme overrides.
    $hooks = array(
      'accordion_menu_header__' . str_replace('-', '_', $config['delta']),
      'accordion_menu_header__' . str_replace('-', '_', $config['menu_name']),
      'accordion_menu_header',
    );
    $element['attributes']['class'] = $class;
    $element['title'] = $data['link']['title'];
    $element['href'] = $data['link']['href'];
    $element['localized_options'] = !empty($data['link']['localized_options']) ? $data['link']['localized_options'] : array();
    $element['below'] = $subtree ? menu_tree_output($subtree) : $subtree;
    $element['original_link'] = $data['link'];
    $element['bid'] = array('module' => 'accordion_menu', 'delta' => $config['delta']);
    $element['config'] = $config;
    $element['count'] = $i + 1;

    // Output the header and submenu items.
    $output .= theme($hooks, $element);

    if ($data['link']['in_active_trail'] == '1') {
      $active_menu = $i;
    }
  }

  // Add class to handle flash of unstyled content in IE 7 and 8.
  $output = str_replace('ul class="menu"', 'ul class="menu hide-at-first"', $output);

  return $output;
}

/**
 * Returns the tree of items below a menu item.
 *
 * @param array $item
 *   A menu item whose children are to be found.
 *
 * @return array
 *   An associative array of the items below the menu item.
 */
function _accordion_menu_subtree($item) {
  static $index = array();
  static $indexed = array();

  // This looks expensive, but menu_tree_all_data uses static caching.
  $tree = menu_tree_all_data($item['menu_name']);

  // Index the tree to find the path to this item.
  if (!isset($indexed[$item['menu_name']])) {
    $index += _accordion_menu_index($tree);
    $indexed[$item['menu_name']] = TRUE;
  }
  // Traverse the tree.
  foreach ($index[$item['mlid']]['parents'] as $mlid) {
    $key = $index[$mlid]['key'];
    if (!isset($tree[$key])) {
      return array();
    }
    $tree = $tree[$key]['below'];
  }
  $key = $index[$item['mlid']]['key'];
  return isset($tree[$key]) ? $tree[$key]['below'] : array();
}

/**
 * Returns the menu tree indexed by mlid.
 *
 * This is needed to identify the items without relying on titles. This function
 * is recursive.
 *
 * @param array $tree
 *   A tree of menu items such as the return value of menu_tree_all_data()
 *
 * @return
 *   An array associating mlid values with the internal keys of the menu tree.
 */
function _accordion_menu_index($tree, $ancestors = array(), $parent = NULL) {
  $index = array();
  if ($parent) {
    $ancestors[] = $parent;
  }
  foreach ($tree as $key => $item) {
    $index[$item['link']['mlid']] = array(
      'key' => $key,
      'parents' => $ancestors,
    );
    if (!empty($item['below'])) {
      $index += _accordion_menu_index($item['below'], $ancestors, $item['link']['mlid']);
    }
  }
  return $index;
}

/**
 * Returns HTML for the accordion menu header and submenu items.
 *
 * @param $element
 *   An associative array containing the menu object being formatted.
 *
 * @ingroup themeable
 */
function theme_accordion_menu_header($element) {
  $sub_menu = '';
  $header = check_plain($element['config']['header']);
  $header_link = $element['config']['header_link'];

  if ($element['below']) {
    $sub_menu = $element['below'];
  }

  if (!$header_link) {
    // @todo Allow the title to be output without cleansing based on user input.
    // Merge in defaults.
    $options = $element['localized_options'];
    $options += array(
      'attributes' => array(), 
      'html' => FALSE,
    );
    $tag = 'span';
    $link = '<' . $tag . drupal_attributes($options['attributes']) . '>' . ($options['html'] ? $element['title'] : check_plain($element['title'])) . '</' . $tag . '>';
  }
  else {
    $link = l($element['title'], $element['href'], $element['localized_options']);
  }

  // The standard menu rendering nests the sub_menu items beneath the header.
  // For the accordion effect to work, the sub_menu must be wrapped in a "div"
  // following the header element.
  $output = '<' . $header . drupal_attributes($element['attributes']) . '>' . $link . '</' . $header . '>' . "\n";
  return $output . '<div class="accordion-content-' . $element['count'] . '">' . $sub_menu . '</div>' . "\n";
}

/**
 * Process variables for accordion-menu-wrapper.tpl.php.
 *
 * @see accordion-menu-wrapper.tpl.php
 *
 * @ingroup themeable
 */
function template_preprocess_accordion_menu_wrapper(&$variables) {
  $variables['classes_array'][] = 'accordion-menu-' . $variables['config']['delta'];
  $variables['classes_array'][] = 'accordion-menu-name-' . $variables['config']['menu_name'];
  $variables['classes_array'][] = 'accordion-menu-source-' . $variables['config']['menu_source'];
  $variables['classes'] = check_plain(implode(' ', $variables['classes_array']));
}
