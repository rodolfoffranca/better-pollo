<script setup>
import { ref } from 'vue'
import Checkbox from '../../Checkbox.vue'

const props = defineProps({
  platform: String,
  config: Object
})

const config = ref(props.config)
const syncLoading = ref(false)
const syncResult = ref(null)
const syncError = ref(null)

async function forceSync() {
  syncLoading.value = true
  syncResult.value = null
  syncError.value = null
  try {
    const res = await fetch('./api/steam/force_sync', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({})
    })
    const data = await res.json()
    if (res.ok && data.status) {
      syncResult.value = data
    } else {
      syncError.value = data.error || 'Failed to sync with Steam'
    }
  } catch (err) {
    syncError.value = err.message || 'Network error while syncing'
  } finally {
    syncLoading.value = false
  }
}
</script>

<template>
  <div id="steam" class="config-page">
    <!-- Steam Enabled -->
    <Checkbox class="mb-3"
              id="steam_enabled"
              locale-prefix="config"
              v-model="config.steam_enabled"
              default="false"
    ></Checkbox>

    <div v-if="platform === 'windows'">
      <Checkbox class="mb-3"
                id="steam_force_resolution"
                locale-prefix="config"
                v-model="config.steam_force_resolution"
                default="false"
      ></Checkbox>
      <div class="mb-3" v-if="config.steam_force_resolution === 'enabled' || config.steam_force_resolution === true">
        <label for="steam_resolution_profile" class="form-label">{{ $t('config.steam_resolution_profile') }}</label>
        <select id="steam_resolution_profile" class="form-select" v-model="config.steam_resolution_profile">
          <option value="auto">{{ $t('config.steam_resolution_auto') }}</option>
          <option value="unity">Unity</option>
          <option value="unreal">Unreal Engine</option>
          <option value="source">Source / Source 2</option>
        </select>
        <div class="form-text">{{ $t('config.steam_resolution_profile_desc') }}</div>
      </div>
    </div>

    <!-- Steam Auto Sync -->
    <Checkbox class="mb-3"
              id="steam_auto_sync"
              locale-prefix="config"
              v-model="config.steam_auto_sync"
              default="false"
    ></Checkbox>

    <!-- Steam Sync All Installed -->
    <Checkbox class="mb-3"
              id="steam_sync_all_installed"
              locale-prefix="config"
              v-model="config.steam_sync_all_installed"
              default="false"
    ></Checkbox>

    <!-- Recent Games Count -->
    <div class="mb-3" v-if="config.steam_sync_all_installed === 'disabled' || config.steam_sync_all_installed === false">
      <label for="steam_recent_games" class="form-label">{{ $t('config.steam_recent_games') }}</label>
      <input type="number" class="form-control" id="steam_recent_games" v-model.number="config.steam_recent_games" min="0" />
      <div class="form-text">{{ $t('config.steam_recent_games_desc') }}</div>
    </div>

    <!-- Recent Games Max Age Days -->
    <div class="mb-3" v-if="config.steam_sync_all_installed === 'disabled' || config.steam_sync_all_installed === false">
      <label for="steam_recent_max_age_days" class="form-label">{{ $t('config.steam_recent_max_age_days') }}</label>
      <input type="number" class="form-control" id="steam_recent_max_age_days" v-model.number="config.steam_recent_max_age_days" min="0" />
      <div class="form-text">{{ $t('config.steam_recent_max_age_days_desc') }}</div>
    </div>

    <!-- Auto-sync Remove Uninstalled -->
    <Checkbox class="mb-3"
              id="steam_autosync_remove_uninstalled"
              locale-prefix="config"
              v-model="config.steam_autosync_remove_uninstalled"
              default="true"
    ></Checkbox>

    <!-- Include Tools -->
    <Checkbox class="mb-3"
              id="steam_include_tools"
              locale-prefix="config"
              v-model="config.steam_include_tools"
              default="false"
    ></Checkbox>

    <!-- Manual Sync Action Box -->
    <div class="card p-3 my-4">
      <h5 class="card-title">{{ $t('config.steam_actions_title') }}</h5>
      <p class="text-muted small">
        {{ $t('config.steam_actions_desc') }}
      </p>
      <div>
        <button type="button" class="btn btn-outline-primary" @click="forceSync" :disabled="syncLoading">
          <i class="fab fa-steam me-1" :class="{'fa-spin': syncLoading}"></i>
          {{ syncLoading ? $t('config.steam_syncing') : $t('config.steam_sync_now') }}
        </button>
      </div>

      <div class="alert alert-success mt-3 mb-0" v-if="syncResult">
        <b>{{ $t('_common.success') }}</b>
        <span>{{ $t('config.steam_sync_success') }}</span>
        <span class="ms-1" v-if="syncResult.game_count !== undefined">({{ syncResult.importable_game_count ?? syncResult.game_count }} {{ $t('config.steam_games_synced') }})</span>
      </div>

      <div class="alert alert-danger mt-3 mb-0" v-if="syncError">
        <b>{{ $t('_common.error') }}</b> {{ syncError }}
      </div>
    </div>
  </div>
</template>

<style scoped>
</style>
