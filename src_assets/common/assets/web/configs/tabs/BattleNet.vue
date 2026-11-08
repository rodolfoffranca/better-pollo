<script setup>
import { ref, onMounted } from 'vue'
import Checkbox from '../../Checkbox.vue'

const props = defineProps({ platform: String, config: Object })
const config = ref(props.config)
const status = ref(null)
const busy = ref(false)
const error = ref('')
const result = ref(null)

async function refresh() {
  try {
    const response = await fetch('./api/battlenet/status')
    const data = await response.json()
    if (!response.ok || !data.status) throw new Error(data.error || 'Battle.net discovery failed')
    status.value = data
    error.value = ''
  } catch (failure) { error.value = failure.message }
}

async function sync() {
  busy.value = true
  error.value = ''
  result.value = null
  try {
    const response = await fetch('./api/battlenet/force_sync', {
      method: 'POST', headers: { 'Content-Type': 'application/json' }, body: '{}'
    })
    const data = await response.json()
    if (!response.ok || !data.status) throw new Error(data.error || 'Battle.net sync failed')
    result.value = data
    await refresh()
  } catch (failure) { error.value = failure.message }
  finally { busy.value = false }
}

onMounted(refresh)
</script>

<template>
  <div id="battlenet" class="config-page">
    <p class="text-muted">{{ $t('config.battlenet_intro') }}</p>
    <Checkbox class="mb-3" id="battlenet_enabled" locale-prefix="config"
              v-model="config.battlenet_enabled" default="false" />
    <Checkbox class="mb-3" id="battlenet_auto_sync" locale-prefix="config"
              v-model="config.battlenet_auto_sync" default="false" />
    <Checkbox class="mb-3" id="battlenet_start_minimized" locale-prefix="config"
              v-model="config.battlenet_start_minimized" default="true" />
    <Checkbox class="mb-3" id="battlenet_remove_uninstalled" locale-prefix="config"
              v-model="config.battlenet_remove_uninstalled" default="true" />
    <div class="mb-3">
      <label for="battlenet_launch_timeout" class="form-label">{{ $t('config.battlenet_launch_timeout') }}</label>
      <input id="battlenet_launch_timeout" class="form-control" type="number" min="15" max="180"
             v-model.number="config.battlenet_launch_timeout" />
      <div class="form-text">{{ $t('config.battlenet_launch_timeout_desc') }}</div>
    </div>
    <div class="card p-3 my-4">
      <h5>{{ $t('config.battlenet_library') }}</h5>
      <p>{{ $t('config.battlenet_save_first') }}</p>
      <div class="alert alert-warning" v-if="status && !status.available">{{ $t('config.battlenet_unavailable') }}</div>
      <div class="alert alert-warning" v-if="status && !status.complete">{{ $t('config.battlenet_partial') }}</div>
      <p v-if="status && status.available">{{ $t('config.battlenet_found') }}: {{ status.game_count }}</p>
      <ul v-if="status && status.games.length">
        <li v-for="game in status.games" :key="game.id" :title="game.install_dir">{{ game.name }}</li>
      </ul>
      <div>
        <button type="button" class="btn btn-outline-primary me-2" @click="sync" :disabled="busy">
          {{ busy ? $t('config.battlenet_syncing') : $t('config.battlenet_sync_now') }}
        </button>
        <button type="button" class="btn btn-outline-secondary" @click="refresh" :disabled="busy">{{ $t('config.battlenet_rescan') }}</button>
      </div>
      <div class="alert alert-success mt-3 mb-0" v-if="result">{{ $t('config.battlenet_synced') }} ({{ result.game_count }})</div>
      <div class="alert alert-danger mt-3 mb-0" v-if="error">{{ error }}</div>
    </div>
  </div>
</template>
