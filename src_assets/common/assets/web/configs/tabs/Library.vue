<script setup>
import Steam from './Steam.vue'
import BattleNet from './BattleNet.vue'
defineProps({ platform: String, config: Object })
</script>

<template>
  <div id="library" class="config-page">
    <section aria-labelledby="library-steam" class="mb-4">
      <h2 id="library-steam">Steam</h2>
      <Steam :config="config" :platform="platform" />
    </section>
    <section v-if="platform === 'windows'" aria-labelledby="library-battlenet">
      <hr class="my-4" />
      <h2 id="library-battlenet">Battle.net</h2>
      <BattleNet :config="config" :platform="platform" />
    </section>
  </div>
</template>
