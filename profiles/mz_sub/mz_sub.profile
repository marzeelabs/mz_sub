<?php

/**
 * Implements hook_form_FORM_ID_alter() for install_configure_form().
 *
 * Allows the profile to alter the site configuration form.
 */
function mz_sub_form_install_configure_form_alter(&$form, $form_state) {
  // When using Drush, let it set the default password.
  if (drupal_is_cli()) {
    return;
  }

  // Pre-populate the site name with the server name.
  $form['site_information']['site_name']['#default_value'] = t('MZ Sub');

  // Set a default country so we can benefit from it on Address Fields.
  $form['server_settings']['site_default_country']['#default_value'] = 'ES';

  // Use "admin" as the default username.
  $form['admin_account']['account']['name']['#default_value'] = 'admin';

  // Set the default admin password.
  $form['admin_account']['account']['pass']['#value'] = 'admin';

  // Hide Update Notifications.
  $form['update_notifications']['#access'] = FALSE;
}
