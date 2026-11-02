#ifndef ACTIVE_REFERENCE_H
#define ACTIVE_REFERENCE_H

/* Visual reference only. No motor, protocol, sensor, timer or HAL dependencies.
 * Called once by the existing 100 Hz Order_Creat_process, just like the old ramp.
 * Header-only so the existing Keil project needs no source-list changes. */
#include <math.h>
#include <stdint.h>
#include <string.h>

typedef enum {
    ACTV_PAUSE_1 = 0, ACTV_MOVING_TO_2, ACTV_PAUSE_2, ACTV_MOVING_TO_1
} actv_state_t;

typedef enum {
    ACTV_REF_OK = 0,
    ACTV_REF_LEGACY_CYCLE, /* old path never arrives: no finite time to preserve */
    ACTV_REF_INVALID,
    ACTV_REF_TIMING_LIMIT  /* cannot certify the old duration: hold, do not guess */
} actv_ref_status_t;

/* Bounded, cached compatibility calculation, not the trajectory clock.
 * At most 65536 old steps per new segment configuration (655.36 nominal s).
 * Beyond this limit the visual reference holds; no control state is touched. */
#define ACTV_REF_REPLAY_LIMIT 65536u

typedef struct {
    float from, to, speed;
    uint32_t ticks;
    uint8_t valid, status;
} actv_ref_plan_t;

typedef struct {
    uint8_t active, state;
    float pos, vel;           /* deg, deg per original control tick */
    uint16_t pause_ticks;
    uint32_t segment_tick, segment_ticks;
    float segment_from, segment_to;
    uint8_t status;
    actv_ref_plan_t plans[2]; /* separate forward/reverse timings */
} actv_rehab_t;

static inline int actv_ref_pause(float seconds, uint16_t *ticks)
{
    float value = seconds * 100.0f;
    /* Match the original truncation for defined uint16_t conversions. */
    if (!isfinite(value) || value < 0.0f || value >= 65536.0f) return 0;
    *ticks = (uint16_t)value;
    return 1;
}

static inline uint32_t actv_ref_duration(float from, float to, float speed,
                                        uint8_t *status)
{
    float pos = from, vel = 0.0f, anchor_pos = from, anchor_vel = 0.0f;
    float step_speed = speed * 0.01f;
    uint32_t tick, power = 1u, length = 0u;
    *status = ACTV_REF_OK;
    if (!isfinite(from) || !isfinite(to) || !isfinite(to - from)
        || !isfinite(step_speed)) {
        *status = ACTV_REF_INVALID;
        return 0u;
    }
    /* Replay the old float operations, including its minimum step speed.
     * This preserves the duration of every convergent replay within the bound.
     * Brent cycle detection proves a repeat; a timeout is NOT called a cycle. */
    for (tick = 1u; tick <= ACTV_REF_REPLAY_LIMIT; ++tick) {
        float error = to - pos, dir, dist, vmax, decel;
        if (fabsf(error) < 0.01f) return tick; /* includes old arrival-check tick */
        dir = error > 0.0f ? 1.0f : -1.0f;
        dist = fabsf(error);
        vmax = step_speed;
        decel = sqrtf(2.0f * 0.1f * dist);
        if (decel < vmax) vmax = decel;
        if (vmax < 0.01f) vmax = 0.01f;
        vel += 0.1f * dir;
        if (vel > vmax) vel = vmax;
        if (vel < -vmax) vel = -vmax;
        pos += vel;
        if (fabsf(pos - to) < 0.005f) pos = to;
        if (!isfinite(pos) || !isfinite(vel)) {
            *status = ACTV_REF_INVALID;
            return 0u;
        }
        ++length;
        if (pos == anchor_pos && vel == anchor_vel) {
            /* A first crossing is not an arrival in the legacy state machine.
             * Do not invent a movement duration for a non-convergent path.
             * Caller holds only this visual reference, not the motor control. */
            *status = ACTV_REF_LEGACY_CYCLE;
            return 0u;
        }
        if (length == power) {
            anchor_pos = pos; anchor_vel = vel;
            power *= 2u; length = 0u;
        }
    }
    *status = ACTV_REF_TIMING_LIMIT;
    return 0u;
}

static inline float actv_ref_scale(float u)
{
    return u*u*u*(10.0f + u*(-15.0f + 6.0f*u));
}

static inline void actv_ref_sample(float from, float to, uint32_t elapsed,
                                   uint32_t duration, float *pos, float *vel)
{
    float u, d;
    if (duration == 0u || elapsed == 0u) { *pos = from; *vel = 0.0f; return; }
    if (elapsed >= duration) { *pos = to; *vel = 0.0f; return; }
    u = (float)elapsed / (float)duration;
    d = to - from;
    /* Symmetry avoids cancellation/overshoot near u=1 with float32. */
    if (u <= 0.5f) *pos = from + d*actv_ref_scale(u);
    else *pos = to - d*actv_ref_scale(1.0f-u);
    *vel = d/(float)duration * 30.0f*u*u*(1.0f-u)*(1.0f-u);
}

static inline void actv_ref_reset(actv_rehab_t *ctx, float first, float pause)
{
    float previous = isfinite(ctx->pos) ? ctx->pos : 0.0f;
    memset(ctx, 0, sizeof(*ctx));
    ctx->active = 1u;
    ctx->state = ACTV_PAUSE_1;
    ctx->pos = isfinite(first) ? first : previous;
    if (!isfinite(first) || !actv_ref_pause(pause, &ctx->pause_ticks))
        ctx->status = ACTV_REF_INVALID;
}

static inline void actv_ref_tick(actv_rehab_t *ctx, float limit1, float limit2,
                                 float speed, float pause)
{
    uint16_t pause_ticks;
    if (!ctx->active) return;
    if (ctx->state == ACTV_PAUSE_1 || ctx->state == ACTV_PAUSE_2) {
        unsigned slot;
        actv_ref_plan_t *plan;
        float target;
        if (ctx->pause_ticks > 0u) { --ctx->pause_ticks; return; }
        if (!isfinite(limit1) || !isfinite(limit2) || !isfinite(speed)
            || !actv_ref_pause(pause, &pause_ticks)) {
            ctx->status = ACTV_REF_INVALID; return;
        }
        slot = ctx->state == ACTV_PAUSE_1 ? 0u : 1u;
        target = slot == 0u ? limit2 : limit1;
        plan = &ctx->plans[slot];
        if (!plan->valid || plan->from != ctx->pos || plan->to != target
                         || plan->speed != speed) {
            plan->from = ctx->pos; plan->to = target; plan->speed = speed;
            plan->ticks = actv_ref_duration(ctx->pos, target, speed, &plan->status);
            plan->valid = 1u; /* cache failures too: no repeated long replay */
        }
        ctx->status = plan->status;
        if (plan->ticks == 0u) return;
        ctx->segment_from = ctx->pos;
        ctx->segment_to = target;
        ctx->segment_tick = 0u;
        ctx->segment_ticks = plan->ticks;
        ctx->vel = 0.0f;
        ctx->state = slot == 0u ? ACTV_MOVING_TO_2 : ACTV_MOVING_TO_1;
        return; /* preserve the old pause-to-motion transition tick */
    }
    if (ctx->state == ACTV_MOVING_TO_2 || ctx->state == ACTV_MOVING_TO_1) {
        ++ctx->segment_tick;
        actv_ref_sample(ctx->segment_from, ctx->segment_to, ctx->segment_tick,
                        ctx->segment_ticks, &ctx->pos, &ctx->vel);
        if (ctx->segment_tick >= ctx->segment_ticks) {
            ctx->state = ctx->state == ACTV_MOVING_TO_2 ? ACTV_PAUSE_2 : ACTV_PAUSE_1;
            if (actv_ref_pause(pause, &pause_ticks)) ctx->pause_ticks = pause_ticks;
            else { ctx->pause_ticks = 0u; ctx->status = ACTV_REF_INVALID; }
        }
    }
}
#endif
